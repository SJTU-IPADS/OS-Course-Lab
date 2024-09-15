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

#ifndef MM_BUDDY_H
#define MM_BUDDY_H

#include <common/types.h>
#include <common/list.h>
#include <common/lock.h>

#define N_PHYS_MEM_POOLS 2

/* The following two are defined in mm.c and filled by mmparse.c. */
extern paddr_t physmem_map[N_PHYS_MEM_POOLS][2];
extern int physmem_map_num;

/* All the following symbols are only used locally in the mm module. */

/* `struct page` is the metadata of one physical 4k page. */
struct page {
        /* Free list */
        struct list_head node;
        /* Whether the correspond physical page is free now. */
        int allocated;
        /* The order of the memory chunck that this page belongs to. */
        int order;
        /* Used for ChCore slab allocator. */
        void *slab;
        /* The physical memory pool this page belongs to */
        struct phys_mem_pool *pool;
};

struct free_list {
        struct list_head free_list;
        unsigned long nr_free;
};

/*
 * Supported Order: [0, BUDDY_MAX_ORDER).
 * The max allocated size (continous physical memory size) is
 * 2^(BUDDY_MAX_ORDER - 1) * 4K.
 * Given BUDDY_MAX_ORDER is 14, the max allocated chunk is 32M.
 */
#define BUDDY_PAGE_SIZE     (0x1000)
#define BUDDY_MAX_ORDER     (14)

/* One page size is 4K, so the order is 12. */
#define BUDDY_PAGE_SIZE_ORDER (12)

/* Each physical memory chunk can be represented by one physical memory pool. */
struct phys_mem_pool {
        /*
         * The start virtual address (for used in kernel) of
         * this physical memory pool.
         */
        vaddr_t pool_start_addr;
        unsigned long pool_mem_size;

        /*
         * The start virtual address (for used in kernel) of
         * the metadata area of this pool.
         */
        struct page *page_metadata;

        /* One lock for one pool. */
        struct lock buddy_lock;

        /* The free list of different free-memory-chunk orders. */
        struct free_list free_lists[BUDDY_MAX_ORDER];

        /*
         * This field is only used in ChCore unit test.
         * The number of (4k) physical pages in this physical memory pool.
         */
        unsigned long pool_phys_page_num;
};

/* Disjoint physical memory can be represented by several phys_mem_pools. */
extern struct phys_mem_pool global_mem[N_PHYS_MEM_POOLS];


/* All interfaces are kernel/mm module internal interfaces. */

void init_buddy(struct phys_mem_pool *, struct page *start_page,
                vaddr_t start_addr, unsigned long page_num);

struct page *buddy_get_pages(struct phys_mem_pool *, int order);
void buddy_free_pages(struct phys_mem_pool *, struct page *page);

void *page_to_virt(struct page *page);
struct page *virt_to_page(void* ptr);
unsigned long get_free_mem_size_from_buddy(struct phys_mem_pool *);
unsigned long get_total_mem_size_from_buddy(struct phys_mem_pool *);

#endif /* MM_BUDDY_H */
