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

#ifndef OBJECT_CAP_GROUP_H
#define OBJECT_CAP_GROUP_H

#include <object/object.h>
#include <common/list.h>
#include <common/types.h>
#include <common/bitops.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <common/lock.h>
#include <common/hashtable.h>
#include <arch/sync.h>
#ifdef CHCORE_OPENTRUSTEE
#include <common/tee_uuid.h>
#endif

struct object_slot {
	int slot_id;
	struct cap_group *cap_group;
	struct object *object;
	/* link copied slots pointing to the same object */
	struct list_head copies;
	/* rights for object */
	cap_right_t rights;
};

#define BASE_OBJECT_NUM		BITS_PER_LONG
/* 1st cap is cap_group. 2nd cap is vmspace */
#define CAP_GROUP_OBJ_ID	0
#define VMSPACE_OBJ_ID		1

struct slot_table {
	unsigned int slots_size;
	struct object_slot **slots;
	/*
	 * if a bit in full_slots_bmp is 1, corresponding
	 * sizeof(unsigned long) bits in slots_bmp are all set
	 */
	unsigned long *full_slots_bmp;
	unsigned long *slots_bmp;
	struct rwlock table_guard;
};

#define MAX_GROUP_NAME_LEN 63

struct cap_group {
	struct slot_table slot_table;

	/* Proctect thread_list and thread_cnt */
	struct lock threads_lock;
	struct list_head thread_list;
	/* The number of threads */
	int thread_cnt;

	/*
	 * Each process has a unique badge as a global identifier which
	 * is set by the system server, procmgr.
	 * Currently, badge is used as a client ID during IPC.
	 */
	badge_t badge;
	int pid;

	/* Ensures the cap_group_exit function only be executed once */
	int notify_recycler;

	/* A ptrace object that the process attached to */
	void *attached_ptrace;

	/* Now is used for debugging */
	char cap_group_name[MAX_GROUP_NAME_LEN + 1];

	/* Each Process has its own futex status */
	struct lock futex_lock;
	struct htable futex_entries;

#ifdef CHCORE_OPENTRUSTEE
	TEE_UUID uuid;
	size_t heap_size_limit;
#endif /* CHCORE_OPENTRUSTEE */
};

#define current_cap_group (current_thread->cap_group)

/*
 * ATTENTION: These interfaces are for capability internal use.
 * As a cap user, check object.h for interfaces for cap.
 */
int alloc_slot_id(struct cap_group *cap_group);

static inline void free_slot_id(struct cap_group *cap_group, cap_t slot_id)
{
	struct slot_table *slot_table = &cap_group->slot_table;
	clear_bit(slot_id, slot_table->slots_bmp);
	clear_bit(slot_id / BITS_PER_LONG, slot_table->full_slots_bmp);
	slot_table->slots[slot_id] = NULL;
}

static inline struct object_slot *get_slot(struct cap_group *cap_group,
					   cap_t slot_id)
{
	if (slot_id < 0 || slot_id >= cap_group->slot_table.slots_size)
		return NULL;
	return cap_group->slot_table.slots[slot_id];
}

static inline void install_slot(struct cap_group *cap_group, cap_t slot_id,
				struct object_slot *slot)
{
	BUG_ON(!get_bit(slot_id, cap_group->slot_table.slots_bmp));
	cap_group->slot_table.slots[slot_id] = slot;
}

void *get_opaque(struct cap_group *cap_group, cap_t slot_id, bool type_valid, int type);

int __cap_free(struct cap_group *cap_group, cap_t slot_id,
	       bool slot_table_locked, bool copies_list_locked);

struct cap_group *create_root_cap_group(char *, size_t);

void cap_group_deinit(void *ptr);

/* Fixed badge for root process and servers */
#define ROOT_CAP_GROUP_BADGE (1) /* INIT */
#define PROCMGR_BADGE        ROOT_CAP_GROUP_BADGE
#define FSM_BADGE            (2)
#define LWIP_BADGE	     (3)
#define TMPFS_BADGE	     (4)
#define SERVER_BADGE_START         (5)
#define DRIVER_BADGE_START         (100)
#define APP_BADGE_START            (200)

/**
 * Fixed pcid for root process (PROCMGR_PCID) and servers,
 * which is exacly the same to the definition in proc.h.
 */
#define ROOT_PROCESS_PCID     (1)
#define FSM_PCID	      (2)
#define LWIP_PCID	      (3)
#define TMPFS_PCID	      (4)

/* Syscalls */
cap_t sys_create_cap_group(unsigned long cap_group_args_p);

#endif /* OBJECT_CAP_GROUP_H */
