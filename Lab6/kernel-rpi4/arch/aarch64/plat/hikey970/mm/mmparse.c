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

#include <arch/boot.h>
#include <arch/mmu.h>
#include <common/macro.h>
#include <common/types.h>
#include <mm/mm.h>
#include <machine.h>
#include <mm/buddy.h>

/* TODO: get usuable physical memory on Hikey*/
#define NPAGES (240 * 1024)

void parse_mem_map(void *info)
{
        paddr_t free_mem_start;
        paddr_t aligned_addr;

        /* A faked impl for now */
        free_mem_start = ROUND_UP((paddr_t)(&img_end), PAGE_SIZE);

        /* skip the metadata area */
        aligned_addr = free_mem_start + NPAGES * sizeof(struct page);
        aligned_addr = ROUND_UP(aligned_addr, 1 << (BUDDY_MAX_ORDER - 1));

        physmem_map[0][0] = free_mem_start;
        // physmem_map[0][1] = free_mem_start + NPAGES * (PAGE_SIZE +
        // sizeof(struct page));
        physmem_map[0][1] = aligned_addr + NPAGES * PAGE_SIZE;

        physmem_map_num = 1;
}
