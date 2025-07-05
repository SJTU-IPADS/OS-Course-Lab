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

#include <mm/mm.h>
#include <common/kprint.h>
#include <common/macro.h>
#include <mm/slab.h>
#include <mm/buddy.h>
#include <arch/mmu.h>

/* The following two will be filled by parse_mem_map. */
paddr_t physmem_map[N_PHYS_MEM_POOLS][2];
int physmem_map_num;

struct phys_mem_pool global_mem[N_PHYS_MEM_POOLS];

/*
 * The layout of each physmem:
 * | metadata (npages * sizeof(struct page)) | start_vaddr ... (npages *
 * PAGE_SIZE) |
 */
static void init_buddy_for_one_physmem_map(int physmem_map_idx)
{
        paddr_t free_mem_start = 0;
        paddr_t free_mem_end = 0;
        struct page *page_meta_start = NULL;
        unsigned long npages = 0;
        unsigned long npages1 = 0;
        paddr_t free_page_start = 0;

        free_mem_start = physmem_map[physmem_map_idx][0];
        free_mem_end = physmem_map[physmem_map_idx][1];
        kdebug("mem pool %d, free_mem_start: 0x%lx, free_mem_end: 0x%lx\n",
               physmem_map_idx,
               free_mem_start,
               free_mem_end);
#ifdef KSTACK_BASE
        /* KSTACK_BASE should not locate in free_mem_start ~ free_mem_end */
        BUG_ON(KSTACK_BASE >= phys_to_virt(free_mem_start) && KSTACK_BASE < phys_to_virt(free_mem_end));
#endif
        npages = (free_mem_end - free_mem_start)
                 / (PAGE_SIZE + sizeof(struct page));
        free_page_start = ROUND_UP(
                free_mem_start + npages * sizeof(struct page), PAGE_SIZE);

        /* Recalculate npages after alignment. */
        npages1 = (free_mem_end - free_page_start) / PAGE_SIZE;
        npages = npages < npages1 ? npages : npages1;

        page_meta_start = (struct page *)phys_to_virt(free_mem_start);
        kdebug("page_meta_start: 0x%lx, npages: 0x%lx, meta_size: 0x%lx, free_page_start: 0x%lx\n",
               page_meta_start,
               npages,
               sizeof(struct page),
               free_page_start);

        /* Initialize the buddy allocator based on this free memory region. */
        init_buddy(&global_mem[physmem_map_idx],
                   page_meta_start,
                   phys_to_virt(free_page_start),
                   npages);
}

void mm_init(void *physmem_info)
{
        int physmem_map_idx;

        /* Step-1: parse the physmem_info to get each continuous range of the
         * physmem. */
        physmem_map_num = 0;
        parse_mem_map(physmem_info);

        /* Step-2: init the buddy allocators for each continuous range of the
         * physmem. */
        for (physmem_map_idx = 0; physmem_map_idx < physmem_map_num;
             ++physmem_map_idx)
                init_buddy_for_one_physmem_map(physmem_map_idx);

        /* Step-3: init the slab allocator. */
        init_slab();
}

unsigned long get_free_mem_size(void)
{
        unsigned long size;
        int i;

        size = get_free_mem_size_from_slab();
        for (i = 0; i < physmem_map_num; ++i)
                size += get_free_mem_size_from_buddy(&global_mem[i]);

        return size;
}

unsigned long get_total_mem_size(void)
{
        unsigned long size = 0;
        int i;

        for (i = 0; i < physmem_map_num; ++i)
                size += get_total_mem_size_from_buddy(&global_mem[i]);

        return size;
}
