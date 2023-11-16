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

#include <common/util.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <mm/buddy.h>

static struct page *get_buddy_chunk(struct phys_mem_pool *pool,
                                    struct page *chunk)
{
        vaddr_t chunk_addr;
        vaddr_t buddy_chunk_addr;
        int order;

        /* Get the address of the chunk. */
        chunk_addr = (vaddr_t)page_to_virt(chunk);
        order = chunk->order;
        /*
         * Calculate the address of the buddy chunk according to the address
         * relationship between buddies.
         */
        buddy_chunk_addr = chunk_addr
                           ^ (1UL << (order + BUDDY_PAGE_SIZE_ORDER));

        /* Check whether the buddy_chunk_addr belongs to pool. */
        if ((buddy_chunk_addr < pool->pool_start_addr)
            || ((buddy_chunk_addr + (1 << order) * BUDDY_PAGE_SIZE)
                > (pool->pool_start_addr + pool->pool_mem_size))) {
                return NULL;
        }

        return virt_to_page((void *)buddy_chunk_addr);
}

/* The most recursion level of split_chunk is decided by the macro of
 * BUDDY_MAX_ORDER. */
static struct page *split_chunk(struct phys_mem_pool *pool, int order,
                                struct page *chunk)
{
        struct page *buddy_chunk;
        struct list_head *free_list;

        /*
         * If the @chunk's order equals to the required order,
         * return this chunk.
         */
        if (chunk->order == order)
                return chunk;

        /*
         * If the current order is larger than the required order,
         * split the memory chunck into two halves.
         */
        chunk->order -= 1;

        buddy_chunk = get_buddy_chunk(pool, chunk);
        /* The buddy_chunk must exist since we are spliting a large chunk. */
        if (buddy_chunk == NULL) {
                BUG("buddy_chunk must exist");
                return NULL;
        }

        /* Set the metadata of the remaining buddy_chunk. */
        buddy_chunk->order = chunk->order;
        buddy_chunk->allocated = 0;

        /* Put the remaining buddy_chunk into its correspondint free list. */
        free_list = &(pool->free_lists[buddy_chunk->order].free_list);
        list_add(&buddy_chunk->node, free_list);
        pool->free_lists[buddy_chunk->order].nr_free += 1;

        /* Continue to split current chunk (@chunk). */
        return split_chunk(pool, order, chunk);
}

/* The most recursion level of merge_chunk is decided by the macro of
 * BUDDY_MAX_ORDER. */
static struct page *merge_chunk(struct phys_mem_pool *pool, struct page *chunk)
{
        struct page *buddy_chunk;

        /* The @chunk has already been the largest one. */
        if (chunk->order == (BUDDY_MAX_ORDER - 1)) {
                return chunk;
        }

        /* Locate the buddy_chunk of @chunk. */
        buddy_chunk = get_buddy_chunk(pool, chunk);

        /* If the buddy_chunk does not exist, no further merge is required. */
        if (buddy_chunk == NULL)
                return chunk;

        /* The buddy_chunk is not free, no further merge is required. */
        if (buddy_chunk->allocated == 1)
                return chunk;

        /* The buddy_chunk is not free as a whole, no further merge is required.
         */
        if (buddy_chunk->order != chunk->order)
                return chunk;

        /* Remove the buddy_chunk from its current free list. */
        list_del(&(buddy_chunk->node));
        pool->free_lists[buddy_chunk->order].nr_free -= 1;

        /* Merge the two buddies and get a larger chunk @chunk (order+1). */
        buddy_chunk->order += 1;
        chunk->order += 1;
        if (chunk > buddy_chunk)
                chunk = buddy_chunk;

        /* Keeping merging. */
        return merge_chunk(pool, chunk);
}

/*
 * The layout of a phys_mem_pool:
 * | page_metadata are (an array of struct page) | alignment pad | usable memory
 * |
 *
 * The usable memory: [pool_start_addr, pool_start_addr + pool_mem_size).
 */
void init_buddy(struct phys_mem_pool *pool, struct page *start_page,
                vaddr_t start_addr, unsigned long page_num)
{
        int order;
        int page_idx;
        struct page *page;

        BUG_ON(lock_init(&pool->buddy_lock) != 0);

        /* Init the physical memory pool. */
        pool->pool_start_addr = start_addr;
        pool->page_metadata = start_page;
        pool->pool_mem_size = page_num * BUDDY_PAGE_SIZE;
        /* This field is for unit test only. */
        pool->pool_phys_page_num = page_num;

        /* Init the free lists */
        for (order = 0; order < BUDDY_MAX_ORDER; ++order) {
                pool->free_lists[order].nr_free = 0;
                init_list_head(&(pool->free_lists[order].free_list));
        }

        /* Clear the page_metadata area. */
        memset((char *)start_page, 0, page_num * sizeof(struct page));

        /* Init the page_metadata area. */
        for (page_idx = 0; page_idx < page_num; ++page_idx) {
                page = start_page + page_idx;
                page->allocated = 1;
                page->order = 0;
                page->pool = pool;
        }

        /* Put each physical memory page into the free lists. */
        for (page_idx = 0; page_idx < page_num; ++page_idx) {
                page = start_page + page_idx;
                buddy_free_pages(pool, page);
        }
}

struct page *buddy_get_pages(struct phys_mem_pool *pool, int order)
{
        int cur_order;
        struct list_head *free_list;
        struct page *page = NULL;

        if (unlikely(order >= BUDDY_MAX_ORDER)) {
                kwarn("ChCore does not support allocating such too large "
                      "contious physical memory\n");
                return NULL;
        }

        lock(&pool->buddy_lock);

        /* Search a chunk (with just enough size) in the free lists. */
        for (cur_order = order; cur_order < BUDDY_MAX_ORDER; ++cur_order) {
                free_list = &(pool->free_lists[cur_order].free_list);
                if (!list_empty(free_list)) {
                        /* Get a free memory chunck from the free list */
                        page = list_entry(free_list->next, struct page, node);
                        list_del(&page->node);
                        pool->free_lists[cur_order].nr_free -= 1;
                        page->allocated = 1;
                        break;
                }
        }

        if (unlikely(page == NULL)) {
                kdebug("[OOM] No enough memory in memory pool %p\n", pool);
                goto out;
        }

        /*
         * Split the chunk found and return the start part of the chunck
         * which can meet the required size.
         */
        page = split_chunk(pool, order, page);

out:
        unlock(&pool->buddy_lock);
        return page;
}

void buddy_free_pages(struct phys_mem_pool *pool, struct page *page)
{
        int order;
        struct list_head *free_list;

        lock(&pool->buddy_lock);

        BUG_ON(page->allocated == 0);
        /* Mark the chunk @page as free. */
        page->allocated = 0;
        /* Merge the freed chunk. */
        page = merge_chunk(pool, page);

        /* Put the merged chunk into the its corresponding free list. */
        order = page->order;
        free_list = &(pool->free_lists[order].free_list);
        list_add(&page->node, free_list);
        pool->free_lists[order].nr_free += 1;

        unlock(&pool->buddy_lock);
}

void *page_to_virt(struct page *page)
{
        vaddr_t addr;
        struct phys_mem_pool *pool = page->pool;

        BUG_ON(pool == NULL);

        /* page_idx * BUDDY_PAGE_SIZE + start_addr */
        addr = (page - pool->page_metadata) * BUDDY_PAGE_SIZE
               + pool->pool_start_addr;
        return (void *)addr;
}

struct page *virt_to_page(void *ptr)
{
        struct page *page;
        struct phys_mem_pool *pool = NULL;
        vaddr_t addr = (vaddr_t)ptr;
        int i;

        /* Find the corresponding physical memory pool. */
        for (i = 0; i < physmem_map_num; ++i) {
                if (addr >= global_mem[i].pool_start_addr
                    && addr < global_mem[i].pool_start_addr
                                       + global_mem[i].pool_mem_size) {
                        pool = &global_mem[i];
                        break;
                }
        }

        if (pool == NULL) {
                kdebug("invalid pool in %s", __func__);
                return NULL;
        }

        page = pool->page_metadata
               + (((vaddr_t)addr - pool->pool_start_addr) / BUDDY_PAGE_SIZE);
        return page;
}

unsigned long get_free_mem_size_from_buddy(struct phys_mem_pool *pool)
{
        int order;
        struct free_list *list;
        unsigned long current_order_size;
        unsigned long total_size = 0;

        for (order = 0; order < BUDDY_MAX_ORDER; order++) {
                /* 2^order * 4K */
                current_order_size = BUDDY_PAGE_SIZE * (1 << order);
                list = pool->free_lists + order;
                total_size += list->nr_free * current_order_size;

                /* debug : print info about current order */
                kdebug("buddy memory chunk order: %d, size: 0x%lx, num: %d\n",
                       order,
                       current_order_size,
                       list->nr_free);
        }
        return total_size;
}
