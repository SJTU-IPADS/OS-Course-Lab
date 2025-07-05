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
#include <smc_num.h>

BEGIN_FUNC(fast_smc_entry)
        ldr     x0, =SMC_STD_RESPONSE
        mov     x1, #0
        smc     #0
END_FUNC(fast_smc_entry)

BEGIN_FUNC(cpu_on_entry)
        ldr     x0, =SMC_ON_DONE
        mov     x1, #0
        smc     #0
END_FUNC(cpu_on_entry)

BEGIN_FUNC(cpu_off_entry)
        ldr     x0, =SMC_OFF_DONE
        mov     x1, #0
        smc     #0
END_FUNC(cpu_off_entry)

BEGIN_FUNC(cpu_resume_entry)
        ldr     x0, =SMC_RESUME_DONE
        mov     x1, #0
        smc     #0
END_FUNC(cpu_resume_entry)

BEGIN_FUNC(cpu_suspend_entry)
        ldr     x0, =SMC_SUSPEND_DONE
        mov     x1, #0
        smc     #0
END_FUNC(cpu_suspend_entry)

BEGIN_FUNC(fiq_entry)
        ldr     x0, =SMC_FIQ_DONE
        mov     x1, #0
        smc     #0
END_FUNC(fiq_entry)

BEGIN_FUNC(system_off_entry)
        ldr     x0, =SMC_SYSOFF_DONE
        mov     x1, #0
        smc     #0
END_FUNC(system_off_entry)

BEGIN_FUNC(system_reset_entry)
        ldr     x0, =SMC_SYSRST_DONE
        mov     x1, #0
        smc     #0
END_FUNC(system_reset_entry)

BEGIN_FUNC(tz_vectors)
	b	yield_smc_entry
	b	fast_smc_entry
	b	cpu_on_entry
	b	cpu_off_entry
	b	cpu_resume_entry
	b	cpu_suspend_entry
	b	fiq_entry
	b	system_off_entry
	b	system_reset_entry
END_FUNC(tz_vectors)
