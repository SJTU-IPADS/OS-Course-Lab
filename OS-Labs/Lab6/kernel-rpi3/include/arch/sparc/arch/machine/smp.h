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
#include <common/types.h>
#include <machine.h>

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};
#endif

/*
 * The offset in the per_cpu struct, i.e., struct per_cpu_info.
 *
 * IMPORTANT: modify the following offset values after
 * modifying struct per_cpu_info.
 */
#define OFFSET_CURRENT_EXEC_CTX		0x0
#define OFFSET_LOCAL_CPU_STACK		0x4
#define OFFSET_CURRENT_FPU_OWNER	0x8
#define OFFSET_FPU_DISABLE		0xC
#define SIZEOF_CPU_INFO			0x40 // allignment

#ifndef __ASM__
struct per_cpu_info {
	/* The execution context of current thread */
	unsigned long cur_exec_ctx;

	/* Per-CPU stack */
	char *cpu_stack;

	/* struct thread *fpu_owner */
	void *fpu_owner;
	unsigned long fpu_disable;
} __attribute__((aligned(64)));

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

extern volatile char cpu_status[PLAT_CPU_NUM];

unsigned smp_get_cpu_id(void);
unsigned smp_get_ncpu(void);
void enable_smp_cores(void);
void init_per_cpu_info(void);
struct per_cpu_info *get_per_cpu_info(void);

unsigned plat_smp_get_cpu_id(void);
unsigned plat_smp_get_ncpu(void);
bool plat_enable_smp_core(int cpuid);
#endif
