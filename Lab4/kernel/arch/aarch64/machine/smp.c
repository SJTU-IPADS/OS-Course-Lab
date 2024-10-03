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

#include <common/vars.h>
#include <common/kprint.h>
#include <mm/mm.h>
#include <arch/machine/smp.h>
#include <arch/mmu.h>
#include <common/types.h>
#include <arch/tools.h>
#include <irq/ipi.h>

volatile char cpu_status[PLAT_CPU_NUM] = { cpu_hang };

struct per_cpu_info cpu_info[PLAT_CPU_NUM] __attribute__((aligned(64)));
u64 ctr_el0;

void enable_smp_cores(paddr_t boot_flag)
{
	int i = 0;
	long *secondary_boot_flag;

	/* Set current cpu status */
	cpu_status[smp_get_cpu_id()] = cpu_run;
	secondary_boot_flag = (long *)phys_to_virt(boot_flag);
	for (i = 0; i < PLAT_CPU_NUM; i++) {
		secondary_boot_flag[i] = 1;
		flush_dcache_area((u64) secondary_boot_flag,
				  (u64) sizeof(u64) * PLAT_CPU_NUM);
		asm volatile ("dsb sy");
		while (cpu_status[i] == cpu_hang)
		;
		kinfo("CPU %d is active\n", i);
	}
	/* wait all cpu to boot */
	kinfo("All %d CPUs are active\n", PLAT_CPU_NUM);
	init_ipi_data();
}

inline u32 smp_get_cpu_id(void)
{
	return get_per_cpu_info() - cpu_info;
}

inline struct per_cpu_info *get_per_cpu_info(void)
{
	struct per_cpu_info *info;

	asm volatile ("mrs %0, tpidr_el1":"=r" (info));

	return info;
}

static inline u64 read_ctr(void)
{
	u64 reg;

	asm volatile("mrs %0, ctr_el0":"=r" (reg)::"memory");
	return reg;
}

void init_per_cpu_info(u32 cpuid)
{
	struct per_cpu_info *info;

	if (cpuid == 0)
		ctr_el0 = read_ctr();

	info = &cpu_info[cpuid];

	info->cur_exec_ctx = 0;

	info->cpu_stack = (char *)(KSTACKx_ADDR(cpuid) + CPU_STACK_SIZE);

	info->fpu_owner = NULL;
	info->fpu_disable = 0;

	asm volatile("msr tpidr_el1, %0"::"r" (info));
}

u64 smp_get_mpidr(void)
{
	u64 mpidr = 0;

	asm volatile ("mrs %0, mpidr_el1":"=r" (mpidr));
	return mpidr;
}
