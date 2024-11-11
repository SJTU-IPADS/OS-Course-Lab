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

#include <common/macro.h>
#include "image.h"
#include "boot.h"
#include "consts.h"

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
u64 boot_ttbr0_l3[PTP_ENTRIES] ALIGN(PTP_SIZE);

u64 boot_ttbr1_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l2[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l3[PTP_ENTRIES] ALIGN(PTP_SIZE);

#define IS_VALID (1UL << 0)
#define IS_TABLE (1UL << 1)
#define IS_PTE (1UL << 1)

#define PXN            (0x1UL << 53)
#define UXN            (0x1UL << 54)
#define ACCESSED       (0x1UL << 10)
#define NG             (0x1UL << 11)
#define INNER_SHARABLE (0x3UL << 8)
#define NORMAL_MEMORY  (0x4UL << 2)
#define DEVICE_MEMORY  (0x0UL << 2)
#define RDONLY_S       (0x2UL << 6)

#define SIZE_2M (2UL * 1024 * 1024)
#define SIZE_4K (4UL * 1024)

#define GET_L0_INDEX(x) (((x) >> (12 + 9 + 9 + 9)) & 0x1ff)
#define GET_L1_INDEX(x) (((x) >> (12 + 9 + 9)) & 0x1ff)
#define GET_L2_INDEX(x) (((x) >> (12 + 9)) & 0x1ff)
#define GET_L3_INDEX(x) (((x) >> (12)) & 0x1ff)

extern int boot_cpu_stack[PLAT_CPU_NUMBER][INIT_STACK_SIZE];

void init_kernel_pt(void)
{
        u64 vaddr = PHYSMEM_START;

        /* TTBR0_EL1 0-1G */
        boot_ttbr0_l0[GET_L0_INDEX(vaddr)] = ((u64)boot_ttbr0_l1) | IS_TABLE
                                             | IS_VALID | NG;
        boot_ttbr0_l1[GET_L1_INDEX(vaddr)] = ((u64)boot_ttbr0_l2) | IS_TABLE
                                             | IS_VALID | NG;

	boot_ttbr0_l2[GET_L2_INDEX(vaddr)] = ((u64)boot_ttbr0_l3) | IS_TABLE
                                             | IS_VALID | NG;

	/* first 2M, including .init section */
        for (; vaddr < SIZE_2M; vaddr += SIZE_4K) {
                boot_ttbr0_l3[GET_L3_INDEX(vaddr)] =
                        (vaddr)
                        | UXN /* Unprivileged execute never */
                        | PXN /* Privileged execute never */
                        | ACCESSED /* Set access flag */
                        | NG /* Mark as not global */
                        | INNER_SHARABLE /* Sharebility */
                        | NORMAL_MEMORY /* Normal memory */
                        | IS_PTE
                        | IS_VALID;
                
                /*
                 * Code in init section(img_start~init_end) should be mmaped as
                 * RDONLY_S due to WXN
                 * The boot_cpu_stack is also in the init section, but should 
                 * have write permission
                 */
                if (vaddr >= (u64)(&img_start) && vaddr < (u64)(&init_end) && 
                        (vaddr < (u64)boot_cpu_stack || vaddr >= ((u64)boot_cpu_stack) + PLAT_CPU_NUMBER * INIT_STACK_SIZE)) {
                        boot_ttbr0_l3[GET_L3_INDEX(vaddr)] &= ~PXN;
                        boot_ttbr0_l3[GET_L3_INDEX(vaddr)] |= RDONLY_S; /* Read Only*/
                }
        }

        /* Normal memory: PHYSMEM_START ~ PERIPHERAL_BASE */
        /* Map with 2M granularity */
        for (; vaddr < PERIPHERAL_BASE; vaddr += SIZE_2M) {
                boot_ttbr0_l2[GET_L2_INDEX(vaddr)] =
                        (vaddr) /* low mem, va = pa */
                        | UXN /* Unprivileged execute never */
                        | ACCESSED /* Set access flag */
                        | NG /* Mark as not global */
                        | INNER_SHARABLE /* Sharebility */
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
        /* BLANK BEGIN */
        vaddr = KERNEL_VADDR + PHYSMEM_START;
        boot_ttbr1_l0[GET_L0_INDEX(vaddr)] = ((u64)boot_ttbr1_l1) | IS_TABLE
                                             | IS_VALID;
        boot_ttbr1_l1[GET_L1_INDEX(vaddr)] = ((u64)boot_ttbr1_l2) | IS_TABLE
                                             | IS_VALID;

        /* Normal memory: PHYSMEM_START ~ PERIPHERAL_BASE
	 * The text section code in kernel should be mapped with flag R/X.
	 * The other section and normal memory is mapped with flag R/W.
         * memory layout :
         * | normal memory | kernel text section | kernel data section ... | normal memory |
         */


        boot_ttbr1_l2[GET_L2_INDEX(vaddr)] = ((u64)boot_ttbr1_l3) | IS_TABLE
                                             | IS_VALID;

	/* the kernel text section was mapped in the first 
	 * L2 page table in boot_ptd_l1 now.
	 */
        BUG_ON((u64)(&_text_end) >= KERNEL_VADDR + SIZE_2M);
        /* _text_start & _text_end should be 4K aligned*/
        BUG_ON((u64)(&_text_start) % SIZE_4K != 0 || (u64)(&_text_end) % SIZE_4K != 0);

        for (; vaddr < KERNEL_VADDR + SIZE_2M; vaddr += SIZE_4K) {
                boot_ttbr1_l3[GET_L3_INDEX(vaddr)] =
                        (vaddr - KERNEL_VADDR)
                        | UXN /* Unprivileged execute never */
                        | PXN /* Priviledged execute never*/
                        | ACCESSED /* Set access flag */
                        | INNER_SHARABLE /* Sharebility */
                        | NORMAL_MEMORY /* Normal memory */
                        | IS_PTE
                        | IS_VALID;
                /* (KERNEL_VADDR + TEXT_START ~ KERNEL_VADDR + TEXT_END) was mapped to 
                * physical address (PHY_START ~ PHY_START + TEXT_END) with R/X
                */
                if (vaddr >= (u64)(&_text_start) && vaddr < (u64)(&_text_end)) {
                        boot_ttbr1_l3[GET_L3_INDEX(vaddr)] &= ~PXN;
                        boot_ttbr1_l3[GET_L3_INDEX(vaddr)] |= RDONLY_S; /* Read Only*/
                }
        }

        for (; vaddr < KERNEL_VADDR + PERIPHERAL_BASE; vaddr += SIZE_2M) {
                /* No NG bit here since the kernel mappings are shared */
                boot_ttbr1_l2[GET_L2_INDEX(vaddr)] =
                        (vaddr - KERNEL_VADDR) /* high mem, va = kbase + pa */
                        | UXN /* Unprivileged execute never */
                        | PXN /* Priviledged execute never*/
                        | ACCESSED /* Set access flag */
                        | INNER_SHARABLE /* Sharebility */
                        | NORMAL_MEMORY /* Normal memory */
                        | IS_VALID;
        }

        /* Peripheral memory: PERIPHERAL_BASE ~ PHYSMEM_END */
        /* Map with 2M granularity */
        for (vaddr = KERNEL_VADDR + PERIPHERAL_BASE;
             vaddr < KERNEL_VADDR + PHYSMEM_END;
             vaddr += SIZE_2M) {
                boot_ttbr1_l2[GET_L2_INDEX(vaddr)] =
                        (vaddr - KERNEL_VADDR) /* high mem, va = kbase + pa */
                        | UXN /* Unprivileged execute never */
                        | PXN /* Priviledged execute never*/
                        | ACCESSED /* Set access flag */
                        | DEVICE_MEMORY /* Device memory */
                        | IS_VALID;
        }

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
                                             | PXN      /* Priviledged execute never*/
                                             | ACCESSED /* Set access flag */
                                             | DEVICE_MEMORY /* Device memory */
                                             | IS_VALID;
}
