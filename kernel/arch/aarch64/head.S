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

#include <common/asm.h>
#include <common/vars.h>

.extern empty_page

/* Args in x0 and x1 should be passed to main */
BEGIN_FUNC(start_kernel)

    /* Set the kernel stack at high vaddr */
    ldr     x2, =cpu_stacks
    add     x2, x2, CPU_STACK_SIZE
    mov     sp, x2

    /* Save x0 and x1 */
    stp x0, x1, [sp, #-16]!

    /*
     * Make sure that no translation based
     * on boot page table can happen.
     */
    adrp    x2, empty_page
    msr     ttbr0_el1, x2
    isb

    /*  
     * Call flush_tlb_all here to flush all the cached TLBs for
     * the boot time TTBR0_EL1.
     */
    bl flush_tlb_all

    /* Restore x0 and x1 */
    ldp x0, x1, [sp], #16

    bl      main
END_FUNC(start_kernel)

BEGIN_FUNC(secondary_cpu_boot)
    mov     x19, x0

    mov     x1, #CPU_STACK_SIZE
    mul     x2, x0, x1
    ldr     x3, =cpu_stacks
    add     x2, x2, x3
    add     x2, x2, CPU_STACK_SIZE
    mov     sp, x2

    /*
     * Make sure that no translation based
     * on boot page table can happen.
     */
    adrp    x3, empty_page
    msr     ttbr0_el1, x3
    isb
    /*  
     * Call flush_tlb_all here to flush all the cached TLBs for
     * the boot time TTBR0_EL1.
     */
    bl      flush_tlb_all

    mov     x0, x19
    bl      secondary_start
END_FUNC(secondary_cpu_boot)
