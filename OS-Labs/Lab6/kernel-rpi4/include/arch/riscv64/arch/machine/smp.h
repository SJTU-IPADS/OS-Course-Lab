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

#pragma once

#ifndef __ASM__
#include <common/vars.h>
#include <common/types.h>
#include <machine.h>
#endif

/*
 * The offset in the per_cpu struct, i.e., struct per_cpu_info.
 *
 * IMPORTANT: modify the following offset values after
 * modifying struct per_cpu_info.
 */
#define OFFSET_CURRENT_EXEC_CTX		0
#define OFFSET_LOCAL_CPU_STACK          8
#define OFFSET_CURRENT_FPU_OWNER	16
#define OFFSET_FPU_DISABLE		24
#define OFFSET_LOGICAL_CPU_ID		28
#define OFFSET_HARDWARE_CPU_ID          32
#define OFFSET_USER_T0			36
#define OFFSET_CPU_STATUS               44

#ifndef __ASM__
/*
 * struct per_cpu_info can be obtained by the following code:
 * 		cpu_info[smp_get_cpu_id()]
 */
struct per_cpu_info {
	/*
         * Used for finding the execution context of
         * current thread when handling trap.
         */
        u64 cur_exec_ctx;

        /* Per-CPU stack */
        char *cpu_stack;

        /* struct thread *fpu_owner. */
        void *fpu_owner;
        u32 fpu_disable;

        /* Logical CPU ID, used to implement smp_get_cpu_id(). */
        u32 logical_cpuid;

        u32 hardware_cpuid;

	/* Saved t0 for user threads. */
	u64 t0;

        /* Used for smp boot. */
        volatile u8 cpu_status;
} __attribute__((aligned(64)));

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

extern volatile char cpu_status[PLAT_CPU_NUM];

extern char cpu_stacks[PLAT_CPU_NUM][CPU_STACK_SIZE];

void init_per_cpu_info(u32 hardware_cpuid);
void enable_smp_cores(void);
struct per_cpu_info *get_per_cpu_info(void);
u32 smp_get_cpu_id(void);

#endif