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

#include <arch/mmu.h>
#include <common/macro.h>
#include <common/types.h>
#include <mm/mm.h>
#include <machine.h>
#include <mm/buddy.h>

/*
 * The usable physical memory: 0x8000_0000 - 0xffff_ffff
 * The kernel image starts from 0x9010_0000.
 * We reserve 0x9010_000 ~ 0x9100_0000 for kernel image.
 *
 * So, the first free physical memory chunk we use is 0x8000_000 ~ 0x9010_0000
 * and the free page number (NPAGES_1) should be:
 *     0x1010_0000 / (sizeof(struct page) +  PAGE_SIZE),
 *     i.e., 0x1010_0000 / (40 + 4096) = 65155.7
 *
 * The second free physical memory chunk we use is 0x9100_000 ~ 0xffff_ffff
 * and the free page number (NPAGES_2) should be:
 *     0x6f00_0000 / (sizeof(struct page) +  PAGE_SIZE),
 *     i.e., 0x6f00_0000 / (40 + 4096) = 450258.9
 *
 * TODO: FT2000/4 has two DIMM slots. So more physical memory is available at
 * address 0x020_0000_0000~0xFFF_FFFF_FFFF.
 * For example, if we plug in two DDR4 8G memory, more physical memory is
 * available at address 0x20_0000_0000 ~ 0x23_8000_0000.
 */
#define NPAGES_1 (65100UL)
#define NPAGES_2 (450200UL)

void parse_mem_map(void *info)
{
        paddr_t free_mem_start;
        paddr_t aligned_addr;

        free_mem_start = 0x80000000UL;

        /* First usable memory chunk: 0x9000_0000 ~ 0x9010_0000 */
        /* skip the metadata area */
        aligned_addr = free_mem_start + NPAGES_1 * sizeof(struct page);
        aligned_addr = ROUND_UP(aligned_addr, 1 << (BUDDY_MAX_ORDER - 1));

        physmem_map[0][0] = free_mem_start;
        physmem_map[0][1] = aligned_addr + NPAGES_1 * PAGE_SIZE;

        kinfo("physmem_map start: 0x%lx, physmem_map end: 0x%lx\n",
              physmem_map[0][0],
              physmem_map[0][1]);

#define MAX_SIZE 0x90100000UL // the start address of kernel image
        if (physmem_map[0][1] > MAX_SIZE) {
                BUG("FT2000/4: Physical memory overlaps with the kernel image : use smaller image or set less usuable pages\n");
        }

/* Second usable memory chunk: 0x9100_0000 ~ 0xffff_ffff */
#define SECOND_CHUNK_BASE 0x91000000UL
#define SECOND_CHUNK_END  0xffffffffUL

        free_mem_start = SECOND_CHUNK_BASE;
        /* skip the metadata area */
        aligned_addr = free_mem_start + NPAGES_2 * sizeof(struct page);
        aligned_addr = ROUND_UP(aligned_addr, 1 << (BUDDY_MAX_ORDER - 1));

        physmem_map[1][0] = free_mem_start;
        physmem_map[1][1] = aligned_addr + NPAGES_2 * PAGE_SIZE;
        kinfo("physmem_map start: 0x%lx, physmem_map end: 0x%lx\n",
              physmem_map[1][0],
              physmem_map[1][1]);

        if (physmem_map[1][1] > SECOND_CHUNK_END) {
                BUG("No enough physical memory on FT2000/4: set less usuable pages (1G - 4G)\n");
        }

        physmem_map_num = 2;
}
