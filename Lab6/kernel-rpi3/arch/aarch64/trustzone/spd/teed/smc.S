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
#include <smc_num.h>

.macro switch_to_cpu_stack
	mrs     x24, TPIDR_EL1
	add	x24, x24, #OFFSET_LOCAL_CPU_STACK
	ldr	x24, [x24]
	mov	sp, x24
.endm

BEGIN_FUNC(smc_call)
        smc     #0
END_FUNC(smc_call)

BEGIN_FUNC(yield_smc_entry)
	switch_to_cpu_stack
	b	handle_yield_smc
END_FUNC(yield_smc_entry)

BEGIN_FUNC(cpu_on_entry)
	ldr	x0, =SMC_ON_DONE
	mov	x1, #0
	smc	#0
END_FUNC(cpu_on_entry)

BEGIN_FUNC(cpu_off_entry)
	ldr	x0, =SMC_OFF_DONE
	mov	x1, #0
	smc	#0
END_FUNC(cpu_off_entry)

BEGIN_FUNC(cpu_resume_entry)
	ldr	x0, =SMC_RESUME_DONE
	mov	x1, #0
	smc	#0
END_FUNC(cpu_resume_entry)

BEGIN_FUNC(cpu_suspend_entry)
	ldr	x0, =SMC_SUSPEND_DONE
	mov	x1, #0
	smc	#0
END_FUNC(cpu_suspend_entry)

BEGIN_FUNC(system_off_entry)
	ldr	x0, =SMC_SYSTEM_OFF_DONE
	mov	x1, #0
	smc	#0
END_FUNC(system_off_entry)

BEGIN_FUNC(system_reset_entry)
	ldr	x0, =SMC_SYSTEM_RESET_DONE
	mov	x1, #0
	smc	#0
END_FUNC(system_reset_entry)

BEGIN_FUNC(tz_vectors)
        b       yield_smc_entry
	b       .
	b       cpu_on_entry
	b       cpu_off_entry
	b       cpu_resume_entry
	b       cpu_suspend_entry
	b       .
	b       .
	b       .
	b       .
	b       system_off_entry
	b       system_reset_entry
	b       .
END_FUNC(tz_vectors)
