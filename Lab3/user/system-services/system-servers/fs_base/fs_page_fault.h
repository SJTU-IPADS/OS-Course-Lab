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

#ifndef FS_PAGE_FAULT_H
#define FS_PAGE_FAULT_H

#include <errno.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/mman.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore/memory.h>
#include <chcore/container/list.h>

#include "fs_vnode.h"

/* Same structure in kernel, item of user-level ring buffer */
struct user_fault_msg {
        badge_t fault_badge;
        vaddr_t fault_va;
};

/* Mapping from client mmap area to server vnode structure */
struct fmap_area_mapping {
        badge_t client_badge;
        vaddr_t client_va_start;
        size_t length;

        struct fs_vnode *vnode;
        off_t file_offset;
        u64 flags;
        vmr_prop_t prot;

        struct list_head node;
};

extern struct list_head fmap_area_mappings;
extern pthread_rwlock_t fmap_area_lock;

int fs_page_fault_init(void);

int fmap_area_insert(badge_t client_badge, vaddr_t client_va_start,
                     size_t length, struct fs_vnode *vnode, off_t file_offset,
                     u64 flags, vmr_prop_t prot);
int fmap_area_find(badge_t client_badge, vaddr_t client_va, size_t *area_off,
                   struct fs_vnode **vnode, off_t *file_offset, u64 *flags,
                   vmr_prop_t *prot);
int fmap_area_remove(badge_t client_badge, vaddr_t client_va_start,
                     size_t length);
void fmap_area_recycle(badge_t client_badge);

#endif /* FS_PAGE_FAULT_H */