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

#include <chcore/memory.h>
#include <chcore/container/rbtree_plus.h>
#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/bug.h>
#include <chcore/aslr.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SV39
#define MEM_AUTO_ALLOC_BASE        (0x700000000UL) 
#define MEM_AUTO_ALLOC_REGION_SIZE (0x100000000UL)
#define MEM_SECTION_SIZE           (0x100000000UL)
#else
#define MEM_AUTO_ALLOC_BASE        (0x7000UL << (4 * sizeof(long)))
#define MEM_AUTO_ALLOC_REGION_SIZE (0x1000UL << (4 * sizeof(long)))
#define MEM_SECTION_SIZE           (0x1000UL << (4 * sizeof(long)))
#endif

/**
 * @brief This struct represents a continuous virtual address range
 * [start, start + len) in the process's address space allocated by this module.
 * And all ranges managed by this module are organized into an rbtree plus
 * to provide efficent alloc/free operations.
 */
struct user_vmr {
        struct rbp_node node;
        vaddr_t start;
        vaddr_t len;
};

/**
 * @brief This struct is used to control virtual address region allocation for
 * a userspace process. By default, the total virtual address range managed by
 * this struct is [MEM_AUTO_ALLOC_BASE + rand_off, MEM_AUTO_ALLOC_BASE +
 * rand_off + MEM_AUTO_ALLOC_REGION_SIZE).
 *
 * In ChCore, the usage of virtual address space of a process is controlled by
 * the process itself, and we provide chcore_alloc/free_vaddr in libc to help
 * userspace process to allocate/free non-overlapped continuous virtual address
 * regions. And some of our syscall implementation rely on these interfaces,
 * e.g. mmap. Users can implement their own allocation strategy of course.
 */
struct user_mm_struct {
        pthread_spinlock_t mu;
        struct rbtree_plus user_vmr_tree;
        /**
         * The last known virtual address which is the start address of
         * a free virtual address region.
         */
        vaddr_t free_addr_cache;
        vaddr_t start_addr;
        vaddr_t end_addr;
};

static int cmp_user_vmr_node(const void *key, const struct rbp_node *node)
{
        const struct user_vmr *rhs = container_of(node, struct user_vmr, node);
        const struct user_vmr *lhs = key;

        /**
         * !! Attention: a user_vmr represents a left-closed right-open
         * interval, so we have to use <= and >= to compare them.
         */
        if (lhs->start + lhs->len <= rhs->start) {
                return -1;
        } else if (lhs->start >= rhs->start + rhs->len) {
                return 1;
        } else {
                return 0;
        }
}

static bool less_vmr_node(const struct rbp_node *lhs,
                          const struct rbp_node *rhs)
{
        const struct user_vmr *lhs_vmr =
                container_of(lhs, struct user_vmr, node);
        const struct user_vmr *rhs_vmr =
                container_of(rhs, struct user_vmr, node);
        return lhs_vmr->start + lhs_vmr->len <= rhs_vmr->start;
}

struct user_mm_struct user_mm;
pthread_once_t user_mm_once = PTHREAD_ONCE_INIT;

void user_mm_init(void)
{
        vaddr_t region_start;

        region_start = MEM_AUTO_ALLOC_BASE + ASLR_RAND_OFFSET;

        init_rbtree_plus(&user_mm.user_vmr_tree);
        user_mm.free_addr_cache = region_start;
        pthread_spin_init(&user_mm.mu, 0);
        user_mm.start_addr = region_start;
        /* end before 0x8000... */
        user_mm.end_addr =
                MIN(ROUND_DOWN(region_start + MEM_AUTO_ALLOC_REGION_SIZE,
                               MEM_SECTION_SIZE),
                    LONG_MAX);
}

// clang-format off
/**
 * @brief Search in the virtual address range managed by user_mm_struct to find
 * a free virtual address region with length at least ROUND_UP(size, PAGE_SIZE).
 * 
 * Consider following virtual address range layout after several alloc/free operations:
 *                     free_vaddr_cache
 *                            │
 *                            │
 *                            │
 *                            │
 * ┌──────┬─────────────┬─────▼──────┬────────────────────┬────────┬────────────┬──────────┐
 * │      │             │            │                    │        │            │          │
 * │      │             │            │                    │        │            │          │
 * │  A   │    VMR 1    │     B      │       VMR ...      │   B    │    VMR N   │    C     │
 * │      │             │            │                    │        │            │          │
 * │      │             │            │                    │        │            │          │
 * └──────┴─────────────┴────────────┴────────────────────┴────────┴────────────┴──────────┘
 * To allocate a non-overlapped continuous virtual address region, there are three possible cases,
 * indicated by A,B and C in the diagram above. First, we start from free_vaddr_cache, and search
 * towards user_mm.end_addr. It's possible for us to find an appropriate free region between two
 * allocated regions(case B). We can also figure out that there is enough space after the last
 * allocated region(case C). However, it's still possible that there is no such free region after
 * free_vaddr_cache. If so, we would perform a rewind search, i.e., starting from user_mm.start_addr
 * again, search until we meet free_vaddr_cache(case A). In the end, if we still didn't encounter
 * a feasible free region, this allocation request cannot be satisfied and a NULL would be returned.
 * 
 */
// clang-format on
unsigned long chcore_alloc_vaddr(unsigned long size)
{
        unsigned long ret = 0;
        struct rbp_node *node, *iter;
        struct user_vmr *nearest_vmr, *iter_vmr, *new_vmr;
        struct user_vmr query;
        bool found = false;

        if (!size) {
                return ret;
        }

        pthread_once(&user_mm_once, user_mm_init);
        pthread_spin_lock(&user_mm.mu);

        if (size > user_mm.end_addr - user_mm.start_addr) {
                return ret;
        }
        size = ROUND_UP(size, PAGE_SIZE);

        query.start = user_mm.free_addr_cache;
        query.len = size;
        node = rbp_search_nearest(
                &user_mm.user_vmr_tree, &query, cmp_user_vmr_node);

        // If there isn't any allocated user_vmr, just check it's out of bound
        // or not. If not, allocate and insert it directly.
        if (node == NULL) {
                if (query.start + size > user_mm.end_addr) {
                        // out of bound
                        goto out;
                }
                new_vmr = malloc(sizeof(struct user_vmr));
                
                if (!new_vmr) {
                        ret = 0;
                        goto out;
                }

                new_vmr->start = query.start;
                new_vmr->len = size;
                user_mm.free_addr_cache += size;
                rbp_insert(
                        &user_mm.user_vmr_tree, &new_vmr->node, less_vmr_node);
                ret = new_vmr->start;
                goto out;
        }

        // start from the user_vmr nearest to [free_vaddr_cache,
        // free_vaddr_cache + size)
        nearest_vmr = container_of(node, struct user_vmr, node);
        for (iter = node; iter; iter = rbp_next(&user_mm.user_vmr_tree, iter)) {
                iter_vmr = container_of(iter, struct user_vmr, node);
                if (cmp_user_vmr_node(&query, iter) >= 0) {
                        // go over the iter_vmr.
                        query.start = iter_vmr->start + iter_vmr->len;
                } else {
                        // Case B
                        found = true;
                        break;
                }
        }

        if (!found) {
                // check area after the last user_vmr(iter)
                if (query.start + size <= user_mm.end_addr) {
                        // Case C
                        found = true;
                } else {
                        // rewind search
                        query.start = user_mm.start_addr;
                        rbp_for_each(&user_mm.user_vmr_tree, iter)
                        {
                                iter_vmr = container_of(
                                        iter, struct user_vmr, node);
                                // rewind done, not found
                                if (iter_vmr == nearest_vmr) {
                                        break;
                                }
                                if (cmp_user_vmr_node(&query, iter) >= 0) {
                                        // go over the iter_vmr.
                                        query.start =
                                                iter_vmr->start + iter_vmr->len;
                                } else {
                                        // Case A
                                        found = true;
                                        break;
                                }
                        }
                }
        }

        if (!found) {
                goto out;
        } else {
                new_vmr = malloc(sizeof(struct user_vmr));
                BUG_ON(!new_vmr);
                
                new_vmr->start = query.start;
                new_vmr->len = size;
                rbp_insert(
                        &user_mm.user_vmr_tree, &new_vmr->node, less_vmr_node);
                ret = new_vmr->start;
                user_mm.free_addr_cache = ret + size;
        }

out:
        pthread_spin_unlock(&user_mm.mu);
        return ret;
}

void chcore_free_vaddr(unsigned long vaddr, unsigned long size)
{
        struct rbp_node *iter, *tmp;
        struct user_vmr *vmr;
        struct user_vmr query;
        int updated = 0;

        pthread_once(&user_mm_once, user_mm_init);
        pthread_spin_lock(&user_mm.mu);

        query.start = vaddr;
        query.len = size;

        rbp_for_each_safe(&user_mm.user_vmr_tree, iter, tmp)
        {
                vmr = container_of(iter, struct user_vmr, node);
                if (query.start <= vmr->start
                    && query.start + query.len >= vmr->start + vmr->len) {
                        if (!updated) {
                                user_mm.free_addr_cache = vmr->start;
                                updated = 1;
                        }
                        rbp_erase(&user_mm.user_vmr_tree, iter);
                        free(vmr);
                } else if (cmp_user_vmr_node(&query, iter) == 0) {
                        continue;
                }
        }

        pthread_spin_unlock(&user_mm.mu);
}

void *chcore_auto_map_pmo(cap_t pmo, unsigned long size, unsigned long perm)
{
        unsigned long vaddr = chcore_alloc_vaddr(size);
        if (vaddr == 0) {
                errno = EINVAL;
                return NULL;
        }
        int ret = usys_map_pmo_with_length(pmo, vaddr, perm, size);
        if (ret != 0) {
                chcore_free_vaddr(vaddr, size);
                errno = -ret;
                perror("auto map");
                return NULL;
        }
        return (void *)vaddr;
}

void chcore_auto_unmap_pmo(cap_t pmo, unsigned long vaddr, unsigned long size)
{
        int ret;

        ret = usys_unmap_pmo_with_length(pmo, vaddr, size);
        if (ret)
                printf("invalid parameter!\n");

        chcore_free_vaddr(vaddr, size);
}

void *chcore_alloc_dma_mem(unsigned long size,
                           struct chcore_dma_handle *dma_handle)
{
        int ret;

        if (dma_handle == NULL) {
                return NULL;
        }

        dma_handle->size = size;

        dma_handle->pmo = usys_create_pmo(size, PMO_DATA_NOCACHE);
        if (dma_handle->pmo < 0) {
                return NULL;
        }

        void *res =
                chcore_auto_map_pmo(dma_handle->pmo, size, VM_READ | VM_WRITE);
        if (res == NULL) {
                // TODO: free pmo cap
                return NULL;
        }

        ret = usys_get_phys_addr(res, &dma_handle->paddr);
        if (ret != 0) {
                // TODO: free pmo cap
                return NULL;
        }
        dma_handle->vaddr = (unsigned long)res;

        return res;
}

void chcore_free_dma_mem(struct chcore_dma_handle *dma_handle)
{
        if (dma_handle == NULL || dma_handle->pmo < 0
            || dma_handle->vaddr == 0) {
                printf("dma_handle is invalid\n");
                return;
        }

        chcore_auto_unmap_pmo(
                dma_handle->pmo, dma_handle->vaddr, dma_handle->size);
        usys_revoke_cap(dma_handle->pmo, false);
}
