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

#ifndef OBJECT_OBJECT_H
#define OBJECT_OBJECT_H

#include <common/types.h>
#include <common/errno.h>
#include <common/lock.h>
#include <common/list.h>
#include <uapi/object.h>

struct object {
        u64 type;
        u64 size;
        /* Link all slots point to this object */
        struct list_head copies_head;
        /* Currently only protect copies list */
        struct lock copies_lock;
        /*
         * refcount is added when a slot points to it and when get_object is
         * called. Object is freed when it reaches 0.
         */
        volatile unsigned long refcount;

        /*
         * opaque marks the end of this struct and the real object will be
         * stored here. Now its address will be 8-byte aligned.
         */
        u64 opaque[];
};

enum object_type {
        TYPE_NO_TYPE = 0,
        TYPE_CAP_GROUP,
        TYPE_THREAD,
        TYPE_CONNECTION,
        TYPE_NOTIFICATION,
        TYPE_IRQ,
        TYPE_PMO,
        TYPE_VMSPACE,
#ifdef CHCORE_OPENTRUSTEE
        TYPE_CHANNEL,
        TYPE_MSG_HDL,
#endif /* CHCORE_OPENTRUSTEE */
        TYPE_PTRACE,
        TYPE_NR,
};

/* Whether `r1` contains `r2` under `mask`. */
#define cap_rights_contain(r1, r2, mask) (!(~((r1) & (mask)) & ((r2) & (mask))))
/* Whether `r1` equals `r2` under `mask`. */
#define cap_rights_equal(r1, r2, mask) (((r1) & (mask)) == (r2))
/* Change the bits under `mask` of `r1` into the bits of `r2` under `mask`. */
#define cap_rights_change(old, new, mask) (((old) & ~(mask)) | ((new) & (mask)))

struct cap_group;

typedef void (*obj_deinit_func)(void *);
extern const obj_deinit_func obj_deinit_tbl[TYPE_NR];

void *obj_get(struct cap_group *cap_group, cap_t slot_id, int type);
void *obj_get_with_rights(struct cap_group *cap_group, cap_t slot_id, int type,
                          cap_right_t mask, cap_right_t rights);
void obj_put(void *obj);
void obj_ref(void *obj);

void *obj_alloc(u64 type, u64 size);
void obj_free(void *obj);
void free_object_internal(struct object *object);
cap_t cap_alloc(struct cap_group *cap_group, void *obj);
cap_t cap_alloc_with_rights(struct cap_group *cap_group, void *obj,
                            cap_right_t rights);
int cap_free(struct cap_group *cap_group, cap_t slot_id);
cap_t cap_copy(struct cap_group *src_cap_group,
               struct cap_group *dest_cap_group, cap_t src_slot_id,
               cap_right_t mask, cap_right_t rest);

int cap_free_all(struct cap_group *cap_group, cap_t slot_id);

#define CAP_ALLOC_OBJ_FUNC(name, ...) name(obj, ##__VA_ARGS__)

#define CAP_ALLOC_DECLARE(name, ...)                        \
        cap_t cap_alloc_##name(struct cap_group *cap_group, \
                               cap_right_t rights,          \
                               ##__VA_ARGS__)

#define CAP_ALLOC_IMPL(name, type, size, init, deinit, ...)          \
        cap_t cap_alloc_##name(struct cap_group *cap_group,          \
                               cap_right_t rights,                   \
                               ##__VA_ARGS__)                        \
        {                                                            \
                cap_t ret;                                           \
                void *obj = obj_alloc(type, size);                   \
                if (obj == NULL) {                                   \
                        ret = -ENOMEM;                               \
                        goto out_fail_obj_alloc;                     \
                }                                                    \
                ret = init;                                          \
                if (ret < 0)                                         \
                        goto out_fail_init;                          \
                ret = cap_alloc_with_rights(cap_group, obj, rights); \
                if (ret < 0)                                         \
                        goto out_fail_cap_alloc;                     \
                return ret;                                          \
        out_fail_cap_alloc:                                          \
                deinit;                                              \
        out_fail_init:                                               \
                obj_free(obj);                                       \
        out_fail_obj_alloc:                                          \
                return ret;                                          \
        }

#define CAP_ALLOC_CALL(name, cap_group, rights, ...) \
        cap_alloc_##name(cap_group, rights, ##__VA_ARGS__);

/* Syscalls */
int sys_revoke_cap(cap_t obj_cap, bool revoke_copy);
int sys_transfer_caps(cap_t dest_group_cap, unsigned long src_caps_buf,
                      int nr_caps, unsigned long dst_caps_buf,
                      unsigned long mask_buf, unsigned long rest_buf);

#endif /* OBJECT_OBJECT_H */
