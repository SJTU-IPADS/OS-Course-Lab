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

#ifndef ARCH_AARCH64_ARCH_MACHINE_SMP_H
#define ARCH_AARCH64_ARCH_MACHINE_SMP_H

#ifndef __ASM__
#include <common/vars.h>
#include <machine.h>
#include <common/types.h>

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};
#endif

/*
 * The offset in the per_cpu struct, i.e., struct per_cpu_info.
 * The base addr of this struct is stored in TPIDR_EL1 register.
 *
 * IMPORTANT: modify the following offset values after
 * modifying struct per_cpu_info.
 */
#define OFFSET_CURRENT_EXEC_CTX		0
#define OFFSET_LOCAL_CPU_STACK		8
#define OFFSET_CURRENT_FPU_OWNER	16
#define OFFSET_FPU_DISABLE		24

#ifndef __ASM__
struct per_cpu_info {
	/* The execution context of current thread */
	u64 cur_exec_ctx;

	/* Per-CPU stack */
	char *cpu_stack;

	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;

	char pad[pad_to_cache_line(sizeof(u64) +
				   sizeof(char *) +
				   sizeof(void *) +
				   sizeof(u32))];
} __attribute__((packed, aligned(64)));

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

extern volatile char cpu_status[PLAT_CPU_NUM];

extern u64 ctr_el0;

void enable_smp_cores(paddr_t boot_flag);
void init_per_cpu_info(u32 cpuid);
struct per_cpu_info *get_per_cpu_info(void);
u32 smp_get_cpu_id(void);
u64 smp_get_mpidr(void);
void smp_print_status(u32 cpuid);
#endif

#endif /* ARCH_AARCH64_ARCH_MACHINE_SMP_H */