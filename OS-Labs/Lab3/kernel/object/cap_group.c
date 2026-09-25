/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <object/cap_group.h>
#include <object/thread.h>
#include <object/object.h>
#include <common/list.h>
#include <common/sync.h>
#include <common/util.h>
#include <common/bitops.h>
#include <mm/kmalloc.h>
#include <mm/vmspace.h>
#include <mm/uaccess.h>
#include <lib/printk.h>
#include <ipc/notification.h>
#include <ipc/futex.h>
#include <syscall/syscall_hooks.h>

struct cap_group *root_cap_group;

/* tool functions */
static bool is_valid_slot_id(struct slot_table *slot_table, cap_t slot_id)
{
        if (slot_id < 0 || slot_id >= slot_table->slots_size)
                return false;
        if (!get_bit(slot_id, slot_table->slots_bmp))
                return false;
        if (slot_table->slots[slot_id] == NULL)
                BUG("slot NULL while bmp is not\n");
        return true;
}

static int slot_table_init(struct slot_table *slot_table, unsigned int size)
{
        int r;

        size = DIV_ROUND_UP(size, BASE_OBJECT_NUM) * BASE_OBJECT_NUM;
        slot_table->slots_size = size;
        slot_table->slots = kzalloc(size * sizeof(*slot_table->slots));
        if (!slot_table->slots) {
                r = -ENOMEM;
                goto out_fail;
        }

        slot_table->slots_bmp =
                kzalloc(BITS_TO_LONGS(size) * sizeof(unsigned long));
        if (!slot_table->slots_bmp) {
                r = -ENOMEM;
                goto out_free_slots;
        }

        slot_table->full_slots_bmp = kzalloc(BITS_TO_LONGS(BITS_TO_LONGS(size))
                                             * sizeof(unsigned long));
        if (!slot_table->full_slots_bmp) {
                r = -ENOMEM;
                goto out_free_slots_bmp;
        }

        return 0;
out_free_slots_bmp:
        kfree(slot_table->slots_bmp);
out_free_slots:
        kfree(slot_table->slots);
out_fail:
        return r;
}

struct cap_group_args {
        badge_t badge;
        vaddr_t name;
        unsigned long name_len;
        unsigned long pcid;
        int pid;
#ifdef CHCORE_OPENTRUSTEE
        vaddr_t puuid;
        unsigned long heap_size;
#endif /* CHCORE_OPENTRUSTEE */
};

__maybe_unused static int cap_group_init_common(struct cap_group *cap_group, unsigned int size,
                                 badge_t badge)
{
        struct slot_table *slot_table = &cap_group->slot_table;

        BUG_ON(slot_table_init(slot_table, size));
        rwlock_init(&slot_table->table_guard);
        init_list_head(&cap_group->thread_list);
        lock_init(&cap_group->threads_lock);
        cap_group->thread_cnt = 0;
        cap_group->attached_ptrace = NULL;

        /* Set badge of the new cap group. */
        cap_group->badge = badge;

        /* Set the futex info for the new cap group */
        futex_init(cap_group);

#ifdef CHCORE_OPENTRUSTEE
        cap_group->heap_size_limit = (size_t)-1;
#endif /* CHCORE_OPENTRUSTEE */

        return 0;
}

__maybe_unused static int cap_group_init_user(struct cap_group *cap_group, unsigned int size,
                               struct cap_group_args *args)
{
#ifdef CHCORE_OPENTRUSTEE
        if (check_user_addr_range((vaddr_t)args->puuid, sizeof(TEE_UUID)) != 0)
                return -EINVAL;
#endif /* CHCORE_OPENTRUSTEE */

        if (check_user_addr_range((vaddr_t)args->name, (size_t)args->name_len)
            != 0)
                return -EINVAL;

        cap_group->pid = args->pid;
#ifdef CHCORE_OPENTRUSTEE
        cap_group->heap_size_limit = args->heap_size;
        /* pid used in OH-TEE */
        if (args->puuid) {
                copy_from_user(&cap_group->uuid,
                               (void *)args->puuid,
                               sizeof(TEE_UUID));
        } else {
                memset(&cap_group->uuid, 0, sizeof(TEE_UUID));
        }
#endif /* CHCORE_OPENTRUSTEE */

        cap_group->notify_recycler = 0;
        /* Set the cap_group_name (process_name) for easing debugging */
        memset(cap_group->cap_group_name, 0, MAX_GROUP_NAME_LEN + 1);
        if (args->name_len > MAX_GROUP_NAME_LEN)
                args->name_len = MAX_GROUP_NAME_LEN;

        if (copy_from_user(cap_group->cap_group_name,
                           (void *)args->name,
                           args->name_len)
            != 0) {
                return -EINVAL;
        }
        return cap_group_init_common(cap_group, size, args->badge);
}

void cap_group_deinit(void *ptr)
{
        struct cap_group *cap_group;
        struct slot_table *slot_table;

        cap_group = (struct cap_group *)ptr;
        slot_table = &cap_group->slot_table;
        kfree(slot_table->slots);
        kfree(slot_table->slots_bmp);
        kfree(slot_table->full_slots_bmp);
        futex_deinit(cap_group);
}

/* slot allocation */
static int expand_slot_table(struct slot_table *slot_table)
{
        unsigned int new_size, old_size;
        struct slot_table new_slot_table;
        int r;

        old_size = slot_table->slots_size;
        new_size = old_size + BASE_OBJECT_NUM;
        r = slot_table_init(&new_slot_table, new_size);
        if (r < 0)
                return r;

        memcpy(new_slot_table.slots,
               slot_table->slots,
               old_size * sizeof(*slot_table->slots));
        memcpy(new_slot_table.slots_bmp,
               slot_table->slots_bmp,
               BITS_TO_LONGS(old_size) * sizeof(unsigned long));
        memcpy(new_slot_table.full_slots_bmp,
               slot_table->full_slots_bmp,
               BITS_TO_LONGS(BITS_TO_LONGS(old_size)) * sizeof(unsigned long));
        slot_table->slots_size = new_size;
        kfree(slot_table->slots);
        slot_table->slots = new_slot_table.slots;
        kfree(slot_table->slots_bmp);
        slot_table->slots_bmp = new_slot_table.slots_bmp;
        kfree(slot_table->full_slots_bmp);
        slot_table->full_slots_bmp = new_slot_table.full_slots_bmp;
        return 0;
}

/* should only be called when table_guard is held */
int alloc_slot_id(struct cap_group *cap_group)
{
        int empty_idx = 0, r;
        struct slot_table *slot_table;
        int bmp_size = 0, full_bmp_size = 0;

        slot_table = &cap_group->slot_table;

        while (true) {
                bmp_size = slot_table->slots_size;
                full_bmp_size = BITS_TO_LONGS(bmp_size);

                empty_idx = find_next_zero_bit(
                        slot_table->full_slots_bmp, full_bmp_size, 0);
                if (empty_idx >= full_bmp_size)
                        goto expand;

                empty_idx = find_next_zero_bit(slot_table->slots_bmp,
                                               bmp_size,
                                               empty_idx * BITS_PER_LONG);
                if (empty_idx >= bmp_size)
                        goto expand;
                else
                        break;
        expand:
                r = expand_slot_table(slot_table);
                if (r < 0)
                        goto out_fail;
        }
        BUG_ON(empty_idx < 0 || empty_idx >= bmp_size);

        set_bit(empty_idx, slot_table->slots_bmp);
        if (slot_table->slots_bmp[empty_idx / BITS_PER_LONG]
            == ~((unsigned long)0))
                set_bit(empty_idx / BITS_PER_LONG, slot_table->full_slots_bmp);

        return empty_idx;
out_fail:
        return r;
}

void *get_opaque(struct cap_group *cap_group, cap_t slot_id, bool type_valid,
                 int type, cap_right_t mask, cap_right_t rights)
{
        struct slot_table *slot_table = &cap_group->slot_table;
        struct object_slot *slot;
        void *obj;

        read_lock(&slot_table->table_guard);
        if (!is_valid_slot_id(slot_table, slot_id)) {
                obj = NULL;
                goto out_unlock_table;
        }

        slot = get_slot(cap_group, slot_id);
        BUG_ON(slot == NULL);
        BUG_ON(slot->object == NULL);

        if ((!type_valid || slot->object->type == type)
            && cap_rights_equal(slot->rights, rights, mask)) {
                obj = slot->object->opaque;
        } else {
                obj = NULL;
                goto out_unlock_table;
        }

        atomic_fetch_add_long(&slot->object->refcount, 1);

out_unlock_table:
        read_unlock(&slot_table->table_guard);
        return obj;
}

/* Get an object reference through its cap.
 * The interface will also add the object's refcnt by one.
 */
void *obj_get_with_rights(struct cap_group *cap_group, cap_t slot_id, int type,
                          cap_right_t mask, cap_right_t rights)
{
        return get_opaque(cap_group, slot_id, true, type, mask, rights);
}

void *obj_get(struct cap_group *cap_group, cap_t slot_id, int type)
{
        return obj_get_with_rights(cap_group,
                                   slot_id,
                                   type,
                                   CAP_RIGHT_NO_RIGHTS,
                                   CAP_RIGHT_NO_RIGHTS);
}

/* This is a pair interface of obj_get.
 * Used when no releasing an object reference.
 * The interface will minus the object's refcnt by one.
 *
 * Furthermore, free an object when its reference cnt becomes 0.
 */
void obj_put(void *obj)
{
        struct object *object;
        u64 old_refcount;

        object = container_of(obj, struct object, opaque);
        old_refcount = atomic_fetch_sub_long(&object->refcount, 1);

        if (old_refcount == 1) {
                free_object_internal(object);
        }
}

/*
 * This interface will add an object's refcnt by one.
 * If you do not have the cap of an object, you can
 * use this interface to just claim a reference.
 *
 * Be sure to call obj_put when releasing the reference.
 */
void obj_ref(void *obj)
{
        struct object *object;

        object = container_of(obj, struct object, opaque);
        atomic_fetch_add_long(&object->refcount, 1);
}

cap_t sys_create_cap_group(unsigned long cap_group_args_p)
{
        struct cap_group *new_cap_group;
        struct vmspace *vmspace;
        cap_t cap;
        int r;
        struct cap_group_args args = {0};

        r = hook_sys_create_cap_group(cap_group_args_p);
        if (r != 0)
                return r;

        if (check_user_addr_range((vaddr_t)cap_group_args_p,
                                  sizeof(struct cap_group_args))
            != 0)
                return -EINVAL;

        r = copy_from_user(
                &args, (void *)cap_group_args_p, sizeof(struct cap_group_args));
        if (r) {
                return -EINVAL;
        }

        /* cap current cap_group */
        /* LAB 3 TODO BEGIN */
        /* Allocate a new cap_group object */

        /* LAB 3 TODO END */
        if (!new_cap_group) {
                r = -ENOMEM;
                goto out_fail;
        }
        /* LAB 3 TODO BEGIN */
        /* initialize cap group from user*/

        /* LAB 3 TODO END */

        cap = cap_alloc(current_cap_group, new_cap_group);
        if (cap < 0) {
                r = cap;
                goto out_free_obj_new_grp;
        }

        /* 1st cap is cap_group */
        if (cap_copy(current_thread->cap_group,
                     new_cap_group,
                     cap,
                     CAP_RIGHT_NO_RIGHTS,
                     CAP_RIGHT_NO_RIGHTS)
            != CAP_GROUP_OBJ_ID) {
                kwarn("%s: cap_copy fails or cap[0] is not cap_group\n",
                      __func__);
                r = -ECAPBILITY;
                goto out_free_cap_grp_current;
        }

        /* 2st cap is vmspace */
        /* LAB 3 TODO BEGIN */

        /* LAB 3 TODO END */

        if (!vmspace) {
                r = -ENOMEM;
                goto out_free_obj_vmspace;
        }

        vmspace_init(vmspace, args.pcid);

        r = cap_alloc(new_cap_group, vmspace);
        if (r != VMSPACE_OBJ_ID) {
                kwarn("%s: cap_copy fails or cap[1] is not vmspace\n",
                      __func__);
                r = -ECAPBILITY;
                goto out_free_obj_vmspace;
        }

        return cap;
out_free_obj_vmspace:
        obj_free(vmspace);
out_free_cap_grp_current:
        cap_free(current_cap_group, cap);
        new_cap_group = NULL;
out_free_obj_new_grp:
        obj_free(new_cap_group);
out_fail:
        return r;
}

/* This is for creating the first (init) user process. */
struct cap_group *create_root_cap_group(char *name, size_t name_len)
{
        struct cap_group *cap_group = NULL;
        struct vmspace *vmspace = NULL;
        cap_t slot_id;

        /* LAB 3 TODO BEGIN */
        UNUSED(vmspace);
        UNUSED(cap_group);

        /* LAB 3 TODO END */
        BUG_ON(!cap_group);

        /* LAB 3 TODO BEGIN */
        /* initialize cap group with common, use ROOT_CAP_GROUP_BADGE */

        /* LAB 3 TODO END */
        slot_id = cap_alloc(cap_group, cap_group);

        BUG_ON(slot_id != CAP_GROUP_OBJ_ID);

        /* LAB 3 TODO BEGIN */

        /* LAB 3 TODO END */
        BUG_ON(!vmspace);

        /* fixed PCID 1 for root process, PCID 0 is not used. */
        vmspace_init(vmspace, ROOT_PROCESS_PCID);

        /* LAB 3 TODO BEGIN */

        /* LAB 3 TODO END */

        BUG_ON(slot_id != VMSPACE_OBJ_ID);

        /* Set the cap_group_name (process_name) for easing debugging */
        memset(cap_group->cap_group_name, 0, MAX_GROUP_NAME_LEN + 1);
        if (name_len > MAX_GROUP_NAME_LEN)
                name_len = MAX_GROUP_NAME_LEN;
        memcpy(cap_group->cap_group_name, name, name_len);

        root_cap_group = cap_group;
        return cap_group;
}
