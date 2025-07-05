/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan
 * PSL v1. You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v1 for more details.
 */

#include "image.h"

typedef unsigned long u64;
typedef unsigned int u32;

/* Physical memory address space: 0-1G */
#define PHYSMEM_START   (0x0UL)
#define PERIPHERAL_BASE (0x3F000000UL)
#define PHYSMEM_END     (0x40000000UL)

/* The number of entries in one page table page */
#define PTP_ENTRIES 512
/* The size of one page table page */
#define PTP_SIZE 4096
#define ALIGN(n) __attribute__((__aligned__(n)))
u64 boot_ttbr0_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

u64 boot_ttbr1_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);

#define IS_VALID (1UL << 0)
#define IS_TABLE (1UL << 1)

#define UXN            (0x1UL << 54)
#define ACCESSED       (0x1UL << 10)
#define NG             (0x1UL << 11)
#define INNER_SHARABLE (0x3UL << 8)
#define NORMAL_MEMORY  (0x4UL << 2)
#define DEVICE_MEMORY  (0x0UL << 2)

#define SIZE_2M (2UL * 1024 * 1024)

#define GET_L0_INDEX(x) (((x) >> (12 + 9 + 9 + 9)) & 0x1ff)
#define GET_L1_INDEX(x) (((x) >> (12 + 9 + 9)) & 0x1ff)
#define GET_L2_INDEX(x) (((x) >> (12 + 9)) & 0x1ff)

void init_kernel_pt(void)
{
        u64 vaddr = PHYSMEM_START;

        /* TTBR0_EL1 0-1G */
        boot_ttbr0_l0[GET_L0_INDEX(vaddr)] = ((u64)boot_ttbr0_l1) | IS_TABLE
                                             | IS_VALID | NG;
        boot_ttbr0_l1[GET_L1_INDEX(vaddr)] = ((u64)boot_ttbr0_l2) | IS_TABLE
                                             | IS_VALID | NG;

        /* Normal memory: PHYSMEM_START ~ PERIPHERAL_BASE */
        /* Map with 2M granularity */
        for (; vaddr < PERIPHERAL_BASE; vaddr += SIZE_2M) {
                boot_ttbr0_l2[GET_L2_INDEX(vaddr)] =
                        (vaddr) /* low mem, va = pa */
                        | UXN /* Unprivileged execute never */
                        | ACCESSED /* Set access flag */
                        | NG /* Mark as not global */
                        | INNER_SHARABLE /* Shareability */
                        | NORMAL_MEMORY /* Normal memory */
                        | IS_VALID;
        }

        /* Peripheral memory: PERIPHERAL_BASE ~ PHYSMEM_END */
        /* Map with 2M granularity */
        for (vaddr = PERIPHERAL_BASE; vaddr < PHYSMEM_END; vaddr += SIZE_2M) {
                boot_ttbr0_l2[GET_L2_INDEX(vaddr)] =
                        (vaddr) /* low mem, va = pa */
                        | UXN /* Unprivileged execute never */
                        | ACCESSED /* Set access flag */
                        | NG /* Mark as not global */
                        | DEVICE_MEMORY /* Device memory */
                        | IS_VALID;
        }

        /* TTBR1_EL1 0-1G */
        /* LAB 1 TODO 5 BEGIN */
        /* Step 1: set L0 and L1 page table entry */
        /* BLANK BEGIN */
        /* BLANK END */

        /* Step 2: map PHYSMEM_START ~ PERIPHERAL_BASE with 2MB granularity */
        /* BLANK BEGIN */
        /* BLANK END */

        /* Step 2: map PERIPHERAL_BASE ~ PHYSMEM_END with 2MB granularity */
        /* BLANK BEGIN */
        /* BLANK END */
        /* LAB 1 TODO 5 END */

        /*
         * Local peripherals, e.g., ARM timer, IRQs, and mailboxes
         *
         * 0x4000_0000 .. 0xFFFF_FFFF
         * 1G is enough (for Mini-UART). Map 1G page here.
         */
        vaddr = KERNEL_VADDR + PHYSMEM_END;
        boot_ttbr1_l1[GET_L1_INDEX(vaddr)] = PHYSMEM_END | UXN /* Unprivileged
                                                                  execute never
                                                                */
                                             | ACCESSED /* Set access flag */
                                             | NG /* Mark as not global */
                                             | DEVICE_MEMORY /* Device memory */
                                             | IS_VALID;
}
