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

#ifndef OBJECT_USER_FAULT_H
#define OBJECT_USER_FAULT_H

#include <ipc/notification.h>
#include <common/lock.h>

/* User ring buffer node */
struct user_fault_msg {
        badge_t fault_badge;
        vaddr_t fault_va;
};

/**
 * Save pending thread, and enqueue them when user mapping finished
 */
struct fault_pending_thread {
        /* Use (fault_badge, fault_va) as key to find the pending thread */
        badge_t fault_badge;
        vaddr_t fault_va;

        struct thread *thread;

        struct list_head node;
};

/**
 * A fmap_fault_pool is ownered by a vmspace(cap_group)
 * If thread call sys_user_fault_register,
 * we will create a fmap_fault_pool for the cap_group,
 * and add to fmap_fault_pool_list.
 */
struct fmap_fault_pool {
        badge_t cap_group_badge;
        struct notification *notific;
        struct ring_buffer *msg_buffer_kva;

        /* fault pending thread list */
        struct list_head pending_threads;

        struct lock lock;
        struct list_head node;
};

extern struct lock fmap_fault_pool_list_lock;
extern struct list_head fmap_fault_pool_list;

void handle_user_fault(struct pmobject *pmo, vaddr_t fault_va);

/* Syscalls */
int sys_user_fault_register(cap_t notific_cap, vaddr_t msg_buffer);
int sys_user_fault_map(badge_t client_badge, vaddr_t fault_va, vaddr_t remap_va,
                       bool copy, unsigned long perm);

#endif /* OBJECT_USER_FAULT_H */
