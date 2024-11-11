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
#include <mm/cache.h>

/*
 * The physical memory address map on raspi4 refers to
 * https://datasheets.raspberrypi.org/bcm2711/bcm2711-peripherals.pdf.
 *
 * The available memory area is represented by the diagram below.
 * Note that Chunk 2 and Chunk 3 may not exist according to the size
 * of physical memory.
 * 
 *               img_end                 0x40000000                     0x100000000
 *  |--------------|---------|--------------|---------|---------------------|---------|
 *  | Kernel Image | Chunk 1 |  GPU Memory  | Chunk 2 |  Peripherals Memory | Chunk 3 |
 *  |--------------|---------|--------------|---------|---------------------|---------|
 *  0                  gpu_mem_start             0xFC000000                    phys_mem_end
 * 
*/

#define GPU_MEM_END       0x40000000UL
#define PERIPHERIALS_BASE 0xFC000000UL
#define PERIPHERIALS_END  0x100000000UL

#define SIZE_1M (1UL * 1024 * 1024)
#define GET_MEM_SIZE(board_revision) \
        ((256UL << ((board_revision >> 20UL) & 7UL)) * SIZE_1M)

#ifndef FBINFER
#include <mailbox.h>
static paddr_t get_vc_mem_start(void)
{
        u32 mbox[8];
        // get the board's unique revision code with a mailbox call
        mbox[0] = 8 * 4; // length of the message
        mbox[1] = 0; // this is a request message
        mbox[2] = MBOX_TAG_GetVCMemory; // get video core memory
        mbox[3] = 8; // buffer size
        mbox[4] = 0; // request codes
        mbox[5] = 0; // clear output buffer
        mbox[6] = 0; // clear output buffer
        mbox[7] = 0; // end tag
        arch_flush_cache((vaddr_t)mbox, sizeof(mbox), CACHE_CLEAN_AND_INV);

        /*
         * The following calculation refers to
         * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
         */
        unsigned int r =
                ((((unsigned int)((unsigned long)virt_to_phys((vaddr_t)mbox))
                     & ~0xF))
                 | (8 & 0xF));
        /* wait until we can write to the mailbox */
        do {
                asm volatile("nop");
        } while (*mbox_to_virt(MBOX_STATUS) & MBOX_FULL);
        /* write the address of our message to the mailbox with channel
         * identifier */
        *mbox_to_virt(MBOX_WRITE) = r;
        /* now wait for the response */
        while (1) {
                /* is there a response? */
                do {
                        asm volatile("nop");
                } while (*mbox_to_virt(MBOX_STATUS) & MBOX_EMPTY);
                /* is it a response to our message? */
                if (r == *mbox_to_virt(MBOX_READ))
                        break;
        }
        arch_flush_cache((vaddr_t)mbox, sizeof(mbox), SYNC_IDCACHE);
        paddr_t vc_mem = mbox[5];
        size_t vc_mem_size = mbox[6];
        kinfo("[ChCore] vc_mem: 0x%lx, size: 0x%lx\n", vc_mem, vc_mem_size);
        BUG_ON(vc_mem == 0);
        return vc_mem;
}
#endif /* FBINFER */

void parse_mem_map(void *info)
{
        u32 board_revision;
        paddr_t phys_mem_end;

        /* Chunk 1: */
        physmem_map[0][0] = ROUND_UP((paddr_t)(&img_end), PAGE_SIZE);
        /* All raspi 4b has memory more than 1G.*/
        physmem_map[0][1] = ROUND_DOWN(get_vc_mem_start(), PAGE_SIZE);
        kinfo("physmem_map start: 0x%lx, physmem_map end: 0x%lx\n",
              physmem_map[0][0],
              physmem_map[0][1]);

        board_revision = (u32)(u64)info;
        phys_mem_end = GET_MEM_SIZE(board_revision);
        
        if (phys_mem_end <= GPU_MEM_END) {
        /* The memory is not greater than 1G, just 1 chunk */
                physmem_map_num = 1;
                return;
        }

        /* Chunk 2: */
        physmem_map[1][0] = GPU_MEM_END;
        physmem_map[1][1] =
                MIN(ROUND_DOWN(phys_mem_end, PAGE_SIZE), PERIPHERIALS_BASE);
        kinfo("physmem_map start: 0x%lx, physmem_map end: 0x%lx\n",
              physmem_map[1][0],
              physmem_map[1][1]);

        if (phys_mem_end <= PERIPHERIALS_END) {
        /* The memory is between 1G and 4G, 2 chunks */
                physmem_map_num = 2;
                return;
        }

        /* Chunk 3: */
        physmem_map[2][0] = PERIPHERIALS_END;
        physmem_map[2][1] = ROUND_DOWN(phys_mem_end, PAGE_SIZE);
        kinfo("physmem_map start: 0x%lx, physmem_map end: 0x%lx\n",
              physmem_map[2][0],
              physmem_map[2][1]);
        /* The memory is greater than 4G, 3 chunks. */
        physmem_map_num = 3;
}
