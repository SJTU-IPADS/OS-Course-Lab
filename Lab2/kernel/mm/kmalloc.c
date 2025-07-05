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

#include <common/debug.h>
#include <common/types.h>
#include <common/macro.h>
#include <common/errno.h>
#include <common/util.h>
#include <common/kprint.h>
#include <lib/mem_usage_info_tool.h>

#include <mm/slab.h>
#include <mm/buddy.h>

#define SLAB_MAX_SIZE (1UL << SLAB_MAX_ORDER)
#define ZERO_SIZE_PTR ((void *)(-1UL))

void *_get_pages(int order, bool is_record)
{
        struct page *page = NULL;
        int i;
        void *addr;

        /* Try to get continuous physical memory pages from one physmem pool. */
        for (i = 0; i < physmem_map_num; ++i) {
                page = buddy_get_pages(&global_mem[i], order);
                if (page)
                        break;
        }

        if (unlikely(!page)) {
                kwarn("[OOM] Cannot get page from any memory pool!\n");
                return NULL;
        }

	addr = page_to_virt(page);
#if ENABLE_MEMORY_USAGE_COLLECTING == ON
        if(is_record && collecting_switch) {
                record_mem_usage(BUDDY_PAGE_SIZE << order, addr);
	}
#endif
        return addr;
}

void *get_pages(int order)
{
        return _get_pages(order, true);
}

void _free_pages(void *addr, bool is_revoke_record)
{
        struct page *page;

        page = virt_to_page(addr);
        if (!page) {
                kdebug("invalid page in %s", __func__);
                return;
        }
#if ENABLE_MEMORY_USAGE_COLLECTING == ON
        if (collecting_switch && is_revoke_record) {
                revoke_mem_usage(addr);
	}
#endif
        buddy_free_pages(page->pool, page);
}

void free_pages(void *addr)
{
        _free_pages(addr, true);
}

void free_pages_without_record(void *addr)
{
        _free_pages(addr, false);
}

__maybe_unused static int size_to_page_order(unsigned long size)
{
        unsigned long order;
        unsigned long pg_num;
        unsigned long tmp;

        order = 0;
        pg_num = ROUND_UP(size, BUDDY_PAGE_SIZE) / BUDDY_PAGE_SIZE;
        tmp = pg_num;

        while (tmp > 1) {
                tmp >>= 1;
                order += 1;
        }

        if (pg_num > (1 << order))
                order += 1;

        return (int)order;
}

/* Currently, BUG_ON no available memory. */
void *_kmalloc(size_t size, bool is_record, size_t *real_size)
{
        void *addr = NULL;
        int order;

        if (unlikely(size == 0))
                return ZERO_SIZE_PTR;

        if (size <= SLAB_MAX_SIZE) {
                /* LAB 2 TODO 3 BEGIN */
                /* Step 1: Allocate in slab for small requests. */
                /* BLANK BEGIN */
                UNUSED(addr);
                UNUSED(order);

                /* BLANK END */
#if ENABLE_MEMORY_USAGE_COLLECTING == ON
                if(is_record && collecting_switch) {
                        record_mem_usage(*real_size, addr);
		}
#endif
        } else {
                /* Step 2: Allocate in buddy for large requests. */
                /* BLANK BEGIN */

                /* BLANK END */
                /* LAB 2 TODO 3 END */
        }

        BUG_ON(!addr);
        return addr;
}

void *kmalloc(size_t size)
{
        size_t real_size;
        void *ret;
        ret = _kmalloc(size, true, &real_size);
        return ret;
}

void *kzalloc(size_t size)
{
        void *addr;

        addr = kmalloc(size);
        if (!addr)
                return NULL;
        memset(addr, 0, size);
        return addr;
}

void _kfree(void *ptr, bool is_revoke_record)
{
        struct page *page;

        if (unlikely(ptr == ZERO_SIZE_PTR))
                return;

        page = virt_to_page(ptr);
#if ENABLE_MEMORY_USAGE_COLLECTING == ON
        if (collecting_switch && is_revoke_record) {
                revoke_mem_usage(ptr);
	}
#endif
        if (page && page->slab)
                free_in_slab(ptr);
        else if (page && page->pool)
                buddy_free_pages(page->pool, page);
        else
                kwarn("unexpected state in %s\n", __func__);
}

void kfree(void *ptr)
{
        _kfree(ptr, true);
}
