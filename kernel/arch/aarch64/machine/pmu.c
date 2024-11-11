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

#include <common/types.h>

/* PMCR_EL0 Performance Monitors Control Register */
#define PMCR_EL0_MASK           (0x3f)
#define PMCR_EL0_E              (1 << 0)	/* Enable all counters */
#define PMCR_EL0_P              (1 << 1)	/* Event counter reset */
#define PMCR_EL0_C              (1 << 2)	/* Cycle counter reset */
#define PMCR_EL0_D              (1 << 3)	/* Clock divider */
#define PMCR_EL0_X              (1 << 4)	/* Enable export of events in an IMPLEMENTATION DEFINED event stream */
#define PMCR_EL0_DP             (1 << 5)	/* Disable cycle counter when event counting is prohibited */
#define PMCR_EL0_LC             (1 << 6)	/* Long cycle counter enable */
#define PMCR_EL0_N_SHIFT        (11)	/* An RO field that indicates the number of event counters implemented */
#define PMCR_EL0_N_MASK         (0x1f)

/* PMUSERENR_EL0 Performance Monitors User Enable Register */
#define PMUSERENR_EL0_EN        (1 << 0)	/* Traps EL0 accesses to the Performance Monitors registers to EL1 */
#define PMUSERENR_EL0_SW        (1 << 1)	/* Software Increment write trap control */
#define PMUSERENR_EL0_CR        (1 << 2)	/* Cycle counter read trap control */
#define PMUSERENR_EL0_ER        (1 << 3)	/* Event counter read trap control */


/* PMCNTENSET_EL0 Performance Monitors Count Enable Set register */
#define PMCNTENSET_EL0_C        (1 << 31)       /* PMCCNTR_EL0 enable bit. Enables the cycle counter register. */

void enable_cpu_cnt(void)
{
	asm volatile ("msr pmuserenr_el0, %0"::
		      "r" (PMUSERENR_EL0_EN | PMUSERENR_EL0_SW |
			   PMUSERENR_EL0_CR | PMUSERENR_EL0_ER));
	asm volatile ("msr pmcr_el0, %0"::"r" (PMCR_EL0_LC | PMCR_EL0_E));
	asm volatile ("msr pmcntenset_el0, %0"::"r" (PMCNTENSET_EL0_C));
}

void disable_cpu_cnt(void)
{
	asm volatile ("msr pmcr_el0, %0"::"r" (~PMCR_EL0_E));
	asm volatile ("msr pmcntenset_el0, %0"::"r" (0));
}

void pmu_init(void)
{
        enable_cpu_cnt();
}
