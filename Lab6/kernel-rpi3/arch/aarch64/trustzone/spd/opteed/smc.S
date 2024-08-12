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
#include <arch/machine/smp.h>

.macro switch_to_cpu_stack
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_LOCAL_CPU_STACK
	ldr	x24, [x24]
	mov	sp, x24
.endm

BEGIN_FUNC(smc_call)
	/* switch to SMC page table (va == pa) */
	ldr	x5, =smc_ttbr0_el1
	ldr	x5, [x5]
	msr 	ttbr0_el1, x5
	isb

        smc     #0
END_FUNC(smc_call)

BEGIN_FUNC(yield_smc_entry)
	switch_to_cpu_stack
	b	handle_yield_smc
END_FUNC(yield_smc_entry)
