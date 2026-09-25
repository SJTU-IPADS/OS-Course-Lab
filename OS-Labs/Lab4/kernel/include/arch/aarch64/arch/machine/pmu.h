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

#ifndef ARCH_AARCH64_ARCH_MACHINE_PMU_H
#define ARCH_AARCH64_ARCH_MACHINE_PMU_H

#include <common/types.h>

void enable_cpu_cnt(void);
void disable_cpu_cnt(void);
void pmu_init(void);

static inline u64 pmu_read_real_cycle(void)
{
	s64 tv;
	asm volatile ("mrs %0, pmccntr_el0":"=r" (tv));
	return tv;
}

static inline void pmu_clear_cnt(void)
{
	asm volatile ("msr pmccntr_el0, %0"::"r" (0));
}

#endif /* ARCH_AARCH64_ARCH_MACHINE_PMU_H */