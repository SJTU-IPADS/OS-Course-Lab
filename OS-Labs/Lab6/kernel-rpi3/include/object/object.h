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
	TYPE_CAP_GROUP = 0,
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

struct cap_group;

typedef void (*obj_deinit_func)(void *);
extern const obj_deinit_func obj_deinit_tbl[TYPE_NR];

void *obj_get(struct cap_group *cap_group, cap_t slot_id, int type);
void obj_put(void *obj);
void obj_ref(void *obj);

void *obj_alloc(u64 type, u64 size);
void obj_free(void *obj);
int obj_check_rights(cap_t slot_id, cap_right_t mask, cap_right_t rights);
void free_object_internal(struct object *object);
cap_t cap_alloc(struct cap_group *cap_group, void *obj);
cap_t cap_alloc_with_rights(struct cap_group *cap_group, void *obj, cap_right_t rights);
int cap_free(struct cap_group *cap_group, cap_t slot_id);
cap_t cap_copy(struct cap_group *src_cap_group,
               struct cap_group *dest_cap_group, cap_t src_slot_id, 
	       bool restrict_rights, cap_right_t new_rights);

int cap_free_all(struct cap_group *cap_group, cap_t slot_id);

/* Syscalls */
int sys_revoke_cap(cap_t obj_cap, bool revoke_copy);
int sys_transfer_caps(cap_t dest_group_cap, unsigned long src_caps_buf,
                      int nr_caps, unsigned long dst_caps_buf);

#endif /* OBJECT_OBJECT_H */
