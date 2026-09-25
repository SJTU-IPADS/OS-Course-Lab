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
#include <common/kprint.h>
#include <mm/cache.h>

/*
 * 
 * The available memory area is represented by the diagram below.
 * 
 *   |----------------|----------------|--------------|
 *   | Kernel Image   | Usable Memory  |  GPU Memory  |
 *   |----------------|----------------|--------------|
 *   0            img_end       gpu_mem_start   0x3F000000
 * 
*/


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
        unsigned int r = (((unsigned int)virt_to_phys((vaddr_t)mbox) & ~0xF))
                         | (8 & 0xF);
        /* wait until we can write to the mailbox */
        while (*MBOX_STATUS & MBOX_FULL)
                ;
        /* write the address of our message to the mailbox with channel
         * identifier */
        *MBOX_WRITE = r;
        /* now wait for the response */
        while (1) {
                /* is there a response? */
                while (*MBOX_STATUS & MBOX_EMPTY)
                        ;
                /* is it a response to our message? */
                if (r == *MBOX_READ)
                        break;
        }
        arch_flush_cache((vaddr_t)mbox, sizeof(mbox), SYNC_IDCACHE);
        paddr_t vc_mem = mbox[5];
        size_t vc_mem_size = mbox[6];
        BUG_ON(vc_mem == 0);
        kinfo("[ChCore] vc_mem: 0x%lx, size: 0x%lx\n", vc_mem, vc_mem_size);
        return vc_mem;
}
#endif /* FBINFER */

void parse_mem_map(void *info)
{
        physmem_map_num = 1;
        physmem_map[0][0] = ROUND_UP((paddr_t)(&img_end), PAGE_SIZE);
        physmem_map[0][1] = ROUND_DOWN(get_vc_mem_start(), PAGE_SIZE);
        kinfo("[ChCore] physmem_map: [0x%lx, 0x%lx)\n",
              physmem_map[0][0],
              physmem_map[0][1]);
}
