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

#include <common/kprint.h>
#include <common/types.h>
#include <arch/mmu.h>

#ifndef FBINFER
#include <mailbox.h>

#define PM_RSTC         ((volatile unsigned int*)(MMIO_BASE+0x0010001c))
#define PM_RSTS         ((volatile unsigned int*)(MMIO_BASE+0x00100020))
#define PM_WDOG         ((volatile unsigned int*)(MMIO_BASE+0x00100024))
#define PM_WDOG_MAGIC   0x5a000000
#define PM_RSTC_FULLRST 0x00000020
#define GPFSEL0         ((volatile unsigned int*)(MMIO_BASE+0x00200000))
#define GPFSEL1         ((volatile unsigned int*)(MMIO_BASE+0x00200004))
#define GPFSEL2         ((volatile unsigned int*)(MMIO_BASE+0x00200008))
#define GPFSEL3         ((volatile unsigned int*)(MMIO_BASE+0x0020000C))
#define GPFSEL4         ((volatile unsigned int*)(MMIO_BASE+0x00200010))
#define GPFSEL5         ((volatile unsigned int*)(MMIO_BASE+0x00200014))
#define GPPUD           ((volatile unsigned int*)(MMIO_BASE+0x00200094))
#define GPPUDCLK0       ((volatile unsigned int*)(MMIO_BASE+0x00200098))
#define GPPUDCLK1       ((volatile unsigned int*)(MMIO_BASE+0x0020009C))

void wait_cycles(unsigned int n)
{
        if(n)
                while(n--);
}

void plat_poweroff(void)
{
        unsigned int mbox[8];

        mbox[0] = 8 * 4; // length of the message
        mbox[1] = 0; // this is a request message
        mbox[2] = MBOX_TAG_POWER_STATE; // set power state
        mbox[3] = 8; // buffer size
        mbox[4] = 8; // request codes
        mbox[5] = 0; // device id
        mbox[6] = 0; // bit 0: off, bit 1: no wait
        mbox[7] = 0; // end tag

        unsigned int r = (((unsigned int)virt_to_phys((vaddr_t)mbox) & ~0xF))
                         | (8 & 0xF);

        /* Wait until mailbox is not full */
        while (*MBOX_STATUS & MBOX_FULL);
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

        /* power off gpio pins (but not VCC pins) */
        *GPFSEL0 = 0;
        *GPFSEL1 = 0;
        *GPFSEL2 = 0;
        *GPFSEL3 = 0;
        *GPFSEL4 = 0;
        *GPFSEL5 = 0;
        *GPPUD = 0;
        wait_cycles(150);
        *GPPUDCLK0 = 0xffffffff;
        *GPPUDCLK1 = 0xffffffff;
        wait_cycles(150);
        *GPPUDCLK0 = 0;
        *GPPUDCLK1 = 0;
        
        /* power off the SoC (GPU + CPU) */
        r = *PM_RSTS;
        r &= ~0xfffffaaa;
        r |= 0x555; // partition 63 used to indicate halt
        *PM_RSTS = PM_WDOG_MAGIC | r;
        *PM_WDOG = PM_WDOG_MAGIC | 10;
        *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
}
#endif /* FBINFER */