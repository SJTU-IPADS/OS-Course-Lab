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

#include <stddef.h>
/**
 * User-level lib for handling user page fault on file mapping
 */
#include <time.h>
#include <pthread.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <chcore-internal/fs_debug.h>
#include <chcore/defs.h>
#include <chcore/ring_buffer.h>
#include <chcore/container/list.h>
#include <chcore/memory.h>

#include "fs_page_fault.h"
#include "fs_page_cache.h"
#include "fs_wrapper_defs.h"
#include "fs_vnode.h"

struct ring_buffer *fault_msg_buffer;
#define MAX_MSG_NUM 100
cap_t notific_cap;
struct list_head fmap_area_mappings;
pthread_rwlock_t fmap_area_lock;

/**
 * If page cache module is available,
 *      use addr of page cache page first.
 * Else,
 *      use specific operation defined by under file system (eg. tmpfs)
 * Return (vaddr_t)0 as error.
 */
vaddr_t fs_wrapper_fmap_get_page_addr(struct fs_vnode *vnode, off_t offset)
{
        vaddr_t page_buf;
        off_t page_idx;

        pthread_rwlock_rdlock(&vnode->rwlock);

        assert(offset % PAGE_SIZE == 0);
        if (offset >= ROUND_UP(vnode->size, PAGE_SIZE)) {
                /* out-of-range */
                pthread_rwlock_unlock(&vnode->rwlock);
                return (vaddr_t)0;
        }

        if (using_page_cache) {
                page_idx = offset / PAGE_SIZE;
                page_buf = (vaddr_t)page_cache_get_block_or_page(
                        vnode->page_cache, page_idx, -1, READ);
        } else {
                page_buf =
                        server_ops.fmap_get_page_addr(vnode->private, offset);
        }

        pthread_rwlock_unlock(&vnode->rwlock);
        return (vaddr_t)page_buf;
}

static int handle_one_fault(badge_t fault_badge, vaddr_t fault_va)
{
        vaddr_t server_page_addr;
        size_t area_off;
        struct fs_vnode *vnode;
        off_t file_offset;
        u64 flags;
        vmr_prop_t prot, map_perm = 0;
        bool copy = 0;
        int ret;

        fs_debug_trace_fswrapper(
                "badge=0x%x, va=0x%lx\n", fault_badge, fault_va);

        /* Find mapping area info */
        ret = fmap_area_find(fault_badge,
                             fault_va,
                             &area_off,
                             &vnode,
                             &file_offset,
                             &flags,
                             &prot);
        if (ret < 0) {
                fs_debug_error("ret = %d\n", ret);
                BUG_ON("why a fault happened when not recorded\n");
        }

        fs_debug_trace_fswrapper(
                "fmap_area: area_off=0x%lx, file_off=0x%lx, flags=%ld\n",
                area_off,
                file_offset,
                flags);

        /* Get a server address space page va for mapping client */
        server_page_addr =
                fs_wrapper_fmap_get_page_addr(vnode, file_offset + area_off);
        if (!server_page_addr) {
                /* The file offset is out-of-range */
                fs_debug_warn("vnode->size=0x%lx, offset=0x%lx\n",
                              vnode->size,
                              file_offset + area_off);
        }

        /* Handle flags */
        if (flags & MAP_SHARED) {
                copy = 0;
                map_perm = prot;
                if (!server_page_addr) {
                        /* The file offset is out-of-range */
                        fs_debug_warn("vnode->size=0x%lx, offset=0x%lx\n",
                                      vnode->size,
                                      file_offset + area_off);

                        pthread_rwlock_wrlock(&vnode->rwlock);
                        ret = server_ops.ftruncate(vnode->private,
                                                   file_offset + area_off
                                                           + PAGE_SIZE);
                        if (ret) {
                                goto out_fail;
                        }
                        vnode->size = file_offset + area_off + PAGE_SIZE;
                        server_page_addr = fs_wrapper_fmap_get_page_addr(
                                vnode, file_offset + area_off);
                        if (!server_page_addr) {
                                goto out_fail;
                        }
                        pthread_rwlock_unlock(&vnode->rwlock);
                }
        } else if (flags & MAP_PRIVATE) {
                copy = 0;
                map_perm = VMR_READ | (prot & VMR_EXEC);
                if (!server_page_addr) {
                        /* The file offset is out-of-range */
                        fs_debug_warn("vnode->size=0x%lx, offset=0x%lx\n",
                                      vnode->size,
                                      file_offset + area_off);
                }
        }

        /* Map client page table, and notify fault thread */
        ret = usys_user_fault_map(
                fault_badge, fault_va, server_page_addr, copy, map_perm);
        if (ret < 0) {
                BUG_ON("this call should always be success here\n");
        }

        return 0;
out_fail:
        pthread_rwlock_unlock(&vnode->rwlock);
        return ret;
}

void *user_fault_handler(void *args)
{
        struct user_fault_msg msg;
        int ret;

        while (1) {
                usys_wait(notific_cap, 1 /* Block */, NULL);
                while (get_one_msg(fault_msg_buffer, &msg)) {
                        fs_debug_trace_fswrapper(
                                "fault_msg_slot: 0x%lx | 0x%lx | 0x%lx\n",
                                (vaddr_t)fault_msg_buffer,
                                (u64)msg,
                                (vaddr_t)((void *)fault_msg_buffer
                                          + END_OFFSET));
                        /* Handle msg */
                        ret = handle_one_fault(msg.fault_badge, msg.fault_va);
                        if (ret) {
                                fs_debug_error("ret = %d\n", ret);
                        }
                }
        }
        return NULL;
}

int fs_page_fault_init(void)
{
        int ret;
        pthread_t fh;

        /* Create a ring buffer to recieve kernel fault msg */
        fault_msg_buffer =
                new_ringbuffer(MAX_MSG_NUM, sizeof(struct user_fault_msg));
        if (fault_msg_buffer == 0)
                return -ENOMEM;

        /* Create a notification for fault handler */
        notific_cap = usys_create_notifc();
        if (notific_cap < 0)
                return notific_cap;

        /* Register fmap_fault_pool in kernel using syscall */
        ret = usys_user_fault_register(notific_cap, (vaddr_t)fault_msg_buffer);
        if (ret < 0) {
                free_ringbuffer(fault_msg_buffer);
                return ret;
        }

        /* Init fmap_area_mapping list */
        init_list_head(&fmap_area_mappings);
        pthread_rwlock_init(&fmap_area_lock, NULL);

        /* Create fault handler to do user-level page fault */
        ret = pthread_create(&fh, NULL, user_fault_handler, NULL);
        if (ret < 0) {
                free_ringbuffer(fault_msg_buffer);
                return ret;
        }

        return 0;
}

/**
 * Helpers for fmap_area_mappings
 */

static struct fmap_area_mapping *
create_fmap_mapping(badge_t client_badge, vaddr_t client_va_start,
                    size_t length, struct fs_vnode *vnode, off_t file_offset,
                    u64 flags, vmr_prop_t prot)
{
        struct fmap_area_mapping *mapping;

        mapping = (struct fmap_area_mapping *)malloc(sizeof(*mapping));
        if (!mapping) {
                return NULL;
        }
        mapping->client_badge = client_badge;
        mapping->client_va_start = client_va_start;
        mapping->length = length;
        mapping->vnode = vnode;
        inc_ref_fs_vnode(vnode);
        mapping->file_offset = file_offset;
        mapping->flags = flags;
        mapping->prot = prot;

        return mapping;
}

static void deinit_fmap_mapping(struct fmap_area_mapping *mapping)
{
        dec_ref_fs_vnode(mapping->vnode);
        free(mapping);
}

static bool __is_overlapped_area(struct fmap_area_mapping *new_mapping)
{
        struct fmap_area_mapping *iter;
        if (list_empty(&fmap_area_mappings)) {
                return false;
        }

        for_each_in_list (
                iter, struct fmap_area_mapping, node, &fmap_area_mappings) {
                if (new_mapping->client_badge == iter->client_badge
                    /* StartA <= EndB*/
                    && new_mapping->client_va_start
                               < iter->client_va_start + iter->length
                    /* EndA >= StartB */
                    && new_mapping->client_va_start + new_mapping->length
                               > iter->client_va_start) {
                        return true;
                }
        }

        return false;
}

int fmap_area_insert(badge_t client_badge, vaddr_t client_va_start,
                     size_t length, struct fs_vnode *vnode, off_t file_offset,
                     u64 flags, vmr_prop_t prot)
{
        struct fmap_area_mapping *mapping = create_fmap_mapping(client_badge,
                                                                client_va_start,
                                                                length,
                                                                vnode,
                                                                file_offset,
                                                                flags,
                                                                prot);
        if (!mapping)
                return -ENOMEM;

        fs_debug_trace_fswrapper(
                "client_badge=0x%x, client_va_start=0x%lx, length=%ld\n"
                "vnode->id=%ld, file_offset=%ld, flags=%ld\n",
                client_badge,
                client_va_start,
                length,
                vnode->vnode_id,
                file_offset,
                flags);
        pthread_rwlock_wrlock(&fmap_area_lock);
        if (__is_overlapped_area(mapping)) {
                free(mapping);
                pthread_rwlock_unlock(&fmap_area_lock);
                return -EEXIST;
        }
        list_append(&mapping->node, &fmap_area_mappings);
        pthread_rwlock_unlock(&fmap_area_lock);
        return 0;
}

/**
 * [IN] client_badge, client_va
 * [OUT] area_off, vnode, file_offset, prot
 */
int fmap_area_find(badge_t client_badge, vaddr_t client_va, size_t *area_off,
                   struct fs_vnode **vnode, off_t *file_offset, u64 *flags,
                   vmr_prop_t *prot)
{
        struct fmap_area_mapping *area_iter;
        pthread_rwlock_rdlock(&fmap_area_lock);
        for_each_in_list (area_iter,
                          struct fmap_area_mapping,
                          node,
                          &fmap_area_mappings) {
                if (area_iter->client_badge == client_badge
                    && (area_iter->client_va_start <= client_va)
                    && (area_iter->client_va_start + area_iter->length
                        > client_va)) {
                        /* Hit */
                        *area_off = client_va - area_iter->client_va_start;
                        *vnode = area_iter->vnode;
                        *file_offset = area_iter->file_offset;
                        *flags = area_iter->flags;
                        *prot = area_iter->prot;
                        pthread_rwlock_unlock(&fmap_area_lock);
                        return 0;
                }
        }
        pthread_rwlock_unlock(&fmap_area_lock);

        return -1; /* Not Found */
}

int fmap_area_remove(badge_t client_badge, vaddr_t client_va_start,
                     size_t length)
{
        int ret = -EINVAL;
        struct fmap_area_mapping *area_iter, *tmp;
        pthread_rwlock_rdlock(&fmap_area_lock);
        for_each_in_list_safe (area_iter, tmp, node, &fmap_area_mappings) {
                if (area_iter->client_badge == client_badge
                    && (area_iter->client_va_start == client_va_start)
                    && (area_iter->length == length)) {
                        list_del(&area_iter->node);
                        deinit_fmap_mapping(area_iter);
                        ret = 0;
                        break;
                }
        }
        pthread_rwlock_unlock(&fmap_area_lock);

        return ret;
}

void fmap_area_recycle(badge_t client_badge)
{
        struct fmap_area_mapping *area_iter, *tmp;
        pthread_rwlock_rdlock(&fmap_area_lock);
        for_each_in_list_safe (area_iter, tmp, node, &fmap_area_mappings) {
                if (area_iter->client_badge == client_badge) {
                        list_del(&area_iter->node);
                        deinit_fmap_mapping(area_iter);
                }
        }
        pthread_rwlock_unlock(&fmap_area_lock);
}
