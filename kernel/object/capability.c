/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <machine.h>
#include <common/sync.h>
#include <ipc/connection.h>
#include <ipc/notification.h>
#include <object/memory.h>
#include <object/object.h>
#include <object/cap_group.h>
#include <object/thread.h>
#include <object/irq.h>
#include <object/ptrace.h>
#include <mm/kmalloc.h>
#include <mm/uaccess.h>
#include <mm/vmspace.h>
#include <lib/printk.h>
#include <sched/context.h>

const obj_deinit_func obj_deinit_tbl[TYPE_NR] = {
        [0 ... TYPE_NR - 1] = NULL,
        [TYPE_CAP_GROUP] = cap_group_deinit,
        [TYPE_THREAD] = thread_deinit,
        [TYPE_CONNECTION] = connection_deinit,
        [TYPE_NOTIFICATION] = notification_deinit,
        [TYPE_IRQ] = irq_deinit,
        [TYPE_PMO] = pmo_deinit,
        [TYPE_VMSPACE] = vmspace_deinit,
        [TYPE_PTRACE] = ptrace_deinit
};

/*
 * Usage:
 * obj = obj_alloc(...);
 * initialize the obj;
 * cap_alloc(obj);
 */
void *obj_alloc(u64 type, u64 size)
{
        u64 total_size;
        struct object *object;

        total_size = sizeof(*object) + size;
        object = kzalloc(total_size);
        if (!object)
                return NULL;

        object->type = type;
        object->size = size;
        object->refcount = 0;

        /*
         * If the cap of the object is copied, then the copied cap (slot) is
         * stored in such a list.
         */
        init_list_head(&object->copies_head);
        lock_init(&object->copies_lock);

        return object->opaque;
}

/*
 * After the fail initialization of a cap (after obj_alloc and before
 * cap_alloc), invoke this interface to free the object allocated by obj_alloc.
 */
void obj_free(void *obj)
{
        struct object *object;

        if (!obj)
                return;
        object = container_of(obj, struct object, opaque);

        BUG_ON(object->refcount != 0);
        kfree(object);
}

cap_t cap_alloc(struct cap_group *cap_group, void *obj)
{
        struct object *object;
        struct slot_table *slot_table;
        struct object_slot *slot;
        cap_t r, slot_id;

        object = container_of(obj, struct object, opaque);
        slot_table = &cap_group->slot_table;

        write_lock(&slot_table->table_guard);
        slot_id = alloc_slot_id(cap_group);
        if (slot_id < 0) {
                r = -ENOMEM;
                goto out_unlock_table;
        }

        slot = kmalloc(sizeof(*slot));
        if (!slot) {
                r = -ENOMEM;
                goto out_free_slot_id;
        }
        slot->slot_id = slot_id;
        slot->cap_group = cap_group;
        slot->object = object;
        list_add(&slot->copies, &object->copies_head);

        BUG_ON(object->refcount != 0);
        object->refcount = 1;

        install_slot(cap_group, slot_id, slot);

        write_unlock(&slot_table->table_guard);
        return slot_id;
out_free_slot_id:
        free_slot_id(cap_group, slot_id);
out_unlock_table:
        write_unlock(&slot_table->table_guard);
        return r;
}

#ifndef TEST_OBJECT
/* @object->type == TYPE_THREAD */
static void clear_fpu_owner(struct object *object)
{
        struct thread *thread;
        int cpuid;

        thread = (struct thread *)object->opaque;
        cpuid = thread->thread_ctx->is_fpu_owner;
        /* If is_fpu_owner >= 0, then the thread is the FPU owner of some CPU.
         */
        if (cpuid >= 0) {
                /*
                 * If the thread to free is the FPU owner of some CPU,
                 * then clear the FPU owner on that CPU first.
                 */
                lock(&fpu_owner_locks[cpuid]);
                if (cpu_info[cpuid].fpu_owner == thread)
                        cpu_info[cpuid].fpu_owner = NULL;
                unlock(&fpu_owner_locks[cpuid]);
                thread->thread_ctx->is_fpu_owner = -1;
        }
}
#endif

/* An internal interface: only invoked by __cap_free and obj_put. */
void __free_object(struct object *object)
{
#ifndef TEST_OBJECT
        obj_deinit_func func;

        if (object->type == TYPE_THREAD)
                clear_fpu_owner(object);

        /* Invoke the object-specific free routine */
        func = obj_deinit_tbl[object->type];
        if (func)
                func(object->opaque);
#endif

        BUG_ON(!list_empty(&object->copies_head));
        kfree(object);
}

void free_object_internal(struct object *object)
{
        __free_object(object);
}

/* cap_free (__cap_free) only removes one cap, which differs from cap_free_all.
 */
int __cap_free(struct cap_group *cap_group, cap_t slot_id,
               bool slot_table_locked, bool copies_list_locked)
{
        struct object_slot *slot;
        struct object *object;
        struct slot_table *slot_table;
        int r = 0;
        u64 old_refcount;

        /* Step-1: free the slot_id (i.e., the capability number) in the slot
         * table */
        slot_table = &cap_group->slot_table;
        if (!slot_table_locked && copies_list_locked) {
                /*
                 * Prevent the following deadlock with try_lock():
                 * cap_copy(): read_lock(table_guard) -> lock(copies_lock)
                 * cap_free_all(): lock(copies_lock) -> write_lock(table_guard)
                 */
                if (write_try_lock(&slot_table->table_guard)) {
                        return -EAGAIN;
                }
        } else if (!slot_table_locked) {
                write_lock(&slot_table->table_guard);
        }
        slot = get_slot(cap_group, slot_id);
        if (!slot) {
                r = -ECAPBILITY;
                goto out_unlock_table;
        }

        free_slot_id(cap_group, slot_id);
        if (!slot_table_locked)
                write_unlock(&slot_table->table_guard);

        /* Step-2: remove the slot in the copies-list of the object and free the
         * slot */
        object = slot->object;
        if (copies_list_locked) {
                list_del(&slot->copies);
        } else {
                lock(&object->copies_lock);
                list_del(&slot->copies);
                unlock(&object->copies_lock);
        }
        kfree(slot);

        /* Step-3: decrease the refcnt of the object and free it if necessary */
        old_refcount = atomic_fetch_sub_long(&object->refcount, 1);

        if (old_refcount == 1)
                __free_object(object);

        return 0;

out_unlock_table:
        if (!slot_table_locked)
                write_unlock(&slot_table->table_guard);
        return r;
}

int cap_free(struct cap_group *cap_group, cap_t slot_id)
{
        return __cap_free(cap_group, slot_id, false, false);
}

cap_t cap_copy(struct cap_group *src_cap_group,
               struct cap_group *dest_cap_group, cap_t src_slot_id)
{
        struct object_slot *src_slot, *dest_slot;
        cap_t r, dest_slot_id;
        struct rwlock *src_table_guard, *dest_table_guard;
        bool local_copy;

        struct object *object;

        local_copy = (src_cap_group == dest_cap_group);
        src_table_guard = &src_cap_group->slot_table.table_guard;
        dest_table_guard = &dest_cap_group->slot_table.table_guard;
        if (local_copy) {
                write_lock(dest_table_guard);
        } else {
                /* avoid deadlock */
                while (true) {
                        read_lock(src_table_guard);
                        if (write_try_lock(dest_table_guard) == 0)
                                break;
                        read_unlock(src_table_guard);
                }
        }

        src_slot = get_slot(src_cap_group, src_slot_id);
        if (!src_slot) {
                r = -ECAPBILITY;
                goto out_unlock;
        }

        dest_slot_id = alloc_slot_id(dest_cap_group);
        if (dest_slot_id == -1) {
                r = -ENOMEM;
                goto out_unlock;
        }

        dest_slot = kmalloc(sizeof(*dest_slot));
        if (!dest_slot) {
                r = -ENOMEM;
                goto out_free_slot_id;
        }
        atomic_fetch_add_long(&src_slot->object->refcount, 1);

        dest_slot->slot_id = dest_slot_id;
        dest_slot->cap_group = dest_cap_group;
        dest_slot->object = src_slot->object;

        object = src_slot->object;
        lock(&object->copies_lock);
        list_add(&dest_slot->copies, &src_slot->copies);
        unlock(&object->copies_lock);

        install_slot(dest_cap_group, dest_slot_id, dest_slot);

        write_unlock(dest_table_guard);
        if (!local_copy)
                read_unlock(src_table_guard);
        return dest_slot_id;
out_free_slot_id:
        free_slot_id(dest_cap_group, dest_slot_id);
out_unlock:
        write_unlock(dest_table_guard);
        if (!local_copy)
                read_unlock(src_table_guard);
        return r;
}

/*
 * Free an object points by some cap, which also removes all the caps point to
 * the object.
 */
int cap_free_all(struct cap_group *cap_group, cap_t slot_id)
{
        void *obj;
        struct object *object;
        struct object_slot *slot_iter = NULL, *slot_iter_tmp = NULL;
        int r;

        /*
         * Since obj_get requires to pass the cap type
         * which is not available here, get_opaque is used instead.
         */
        obj = get_opaque(cap_group, slot_id, false, 0);

        if (!obj) {
                r = -ECAPBILITY;
                goto out_fail;
        }

        object = container_of(obj, struct object, opaque);

again:
        /* free all copied slots */
        lock(&object->copies_lock);
        for_each_in_list_safe (
                slot_iter, slot_iter_tmp, copies, &object->copies_head) {
                u64 iter_slot_id = slot_iter->slot_id;
                struct cap_group *iter_cap_group = slot_iter->cap_group;

                r = __cap_free(iter_cap_group, iter_slot_id, false, true);
                if (r == -EAGAIN) {
                        unlock(&object->copies_lock);
                        goto again;
                }
                
                BUG_ON(r != 0);
        }
        unlock(&object->copies_lock);

        /* get_opaque will also add the reference cnt */
        obj_put(obj);

        return 0;

out_fail:
        return r;
}

/* Transfer a number (@nr_caps) of caps from current_cap_group to
 * dest_group_cap. */
int sys_transfer_caps(cap_t dest_group_cap, unsigned long src_caps_buf,
                      int nr_caps, unsigned long dst_caps_buf)
{
        struct cap_group *dest_cap_group;
        int i;
        int *src_caps;
        int *dst_caps;
        size_t size;
        int ret;

#define MAX_TRANSFER_NUM 16
        if ((nr_caps <= 0) || (nr_caps > MAX_TRANSFER_NUM))
                return -EINVAL;

        size = sizeof(int) * nr_caps;
        if ((check_user_addr_range(src_caps_buf, size) != 0)
            || (check_user_addr_range(dst_caps_buf, size) != 0))
                return -EINVAL;

        dest_cap_group =
                obj_get(current_cap_group, dest_group_cap, TYPE_CAP_GROUP);
        if (!dest_cap_group)
                return -ECAPBILITY;

        src_caps = kmalloc(size);
        dst_caps = kmalloc(size);

        /* get args from user buffer @src_caps_buf */
        ret = copy_from_user((void *)src_caps, (void *)src_caps_buf, size);
        if (ret) {
                ret = -EINVAL;
                goto out_obj_put;
        }

        for (i = 0; i < nr_caps; ++i) {
                dst_caps[i] = cap_copy(
                        current_cap_group, dest_cap_group, src_caps[i]);
        }

        /* write results to user buffer @dst_caps_buf */
        ret = copy_to_user((void *)dst_caps_buf, (void *)dst_caps, size);
        if (ret) {
                ret = -EINVAL;
                goto out_obj_put;
        }

        kfree(src_caps);
        kfree(dst_caps);

        obj_put(dest_cap_group);
        return 0;
out_obj_put:
        obj_put(dest_cap_group);
        return ret;
}

int sys_revoke_cap(cap_t obj_cap, bool revoke_copy)
{
        int ret;
        void *obj;

        /*
         * Disallow to revoke the cap of current_cap_group, current_vmspace,
         * or current_thread.
         */
        obj = obj_get(current_cap_group, obj_cap, TYPE_CAP_GROUP);
        if (obj == current_cap_group) {
                ret = -EINVAL;
                goto out_fail;
        }
        if (obj) obj_put(obj);

        obj = obj_get(current_cap_group, obj_cap, TYPE_VMSPACE);
        if (obj == current_thread->vmspace) {
                ret = -EINVAL;
                goto out_fail;
        }
        if (obj) obj_put(obj);

        obj = obj_get(current_cap_group, obj_cap, TYPE_THREAD);
        if (obj == current_thread) {
                ret = -EINVAL;
                goto out_fail;
        }
        if (obj) obj_put(obj);

        if (revoke_copy)
                ret = cap_free_all(current_cap_group, obj_cap);
        else
                ret = cap_free(current_cap_group, obj_cap);
        return ret;

out_fail:
        obj_put(obj);
        return ret;
}
