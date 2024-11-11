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

#include <irq/timer.h>
#include <machine.h>
#include <common/kprint.h>
#include <common/types.h>
#include <arch/machine/smp.h>
#include <sched/sched.h>
#include <arch/tools.h>

/* TODO (tmac): rewrite this file */

u64 cntp_tval;
u64 tick_per_us;
u64 cntp_init;
u64 cntp_freq;

/* Per core IRQ SOURCE MMIO address */
// u64 core_timer_irqcntl[PLAT_CPU_NUM] = {
// 	CORE0_TIMER_IRQCNTL,
// 	CORE1_TIMER_IRQCNTL,
// 	CORE2_TIMER_IRQCNTL,
// 	CORE3_TIMER_IRQCNTL
// };

void plat_timer_init(void)
{
	u64 count_down = 0;
	u64 timer_ctl = 0;
	//u32 cpuid = smp_get_cpu_id();

	/* Since QEMU only emulate the generic timer, we use the generic timer here */
	asm volatile ("mrs %0, cntpct_el0":"=r" (cntp_init));
	kdebug("timer init cntpct_el0 = %lu\n", cntp_init);
	kinfo("timer init cntpct_el0 = %lu\n", cntp_init); // 
	asm volatile ("mrs %0, cntfrq_el0":"=r" (cntp_freq));
	kdebug("timer init cntfrq_el0 = %lu\n", cntp_freq);
	kinfo("timer init cntfrq_el0 = %lu\n", cntp_freq); //

	/* Calculate the tv */
	cntp_tval = (cntp_freq * TICK_MS / 1000);
	tick_per_us = cntp_freq / 1000 / 1000;
	kinfo("CPU freq %lu, set timer %lu\n", cntp_freq, cntp_tval);

	/* set the timervalue here */
	asm volatile ("msr cntp_tval_el0, %0"::"r" (cntp_tval));
	asm volatile ("mrs %0, cntp_tval_el0":"=r" (count_down));
	kinfo("timer init cntp_tval_el0 = %lu\n", count_down);

	/* Enable CNTPNSIRQ. */
	//put32(core_timer_irqcntl[cpuid], CNTPNSIRQ);

	/* Set the control register */
	timer_ctl = 0 << 1 | 1;	/* IMASK = 0 ENABLE = 1 */
	asm volatile ("msr cntp_ctl_el0, %0"::"r" (timer_ctl));
	asm volatile ("mrs %0, cntp_ctl_el0":"=r" (timer_ctl));
	kinfo("timer init cntp_ctl_el0 = %lu\n", timer_ctl);
	/* enable interrupt controller */
	return;
}

void plat_set_next_timer(u64 tick_delta)
{
	// kinfo("%s: tick_delta 0x%lx\n", __func__, tick_delta);
	asm volatile ("msr cntp_tval_el0, %0"::"r" (tick_delta));
}

void plat_handle_timer_irq(u64 tick_delta)
{
	asm volatile ("msr cntp_tval_el0, %0"::"r" (tick_delta));
}

void plat_disable_timer(void)
{
	u64 timer_ctl = 0x0;

	asm volatile ("msr cntp_ctl_el0, %0"::"r" (timer_ctl));
}

void plat_enable_timer(void)
{
	u64 timer_ctl = 0x1;

	asm volatile ("msr cntp_tval_el0, %0"::"r" (cntp_tval));
	asm volatile ("msr cntp_ctl_el0, %0"::"r" (timer_ctl));

}

/* Return the mono time using nano-seconds */
u64 plat_get_mono_time(void)
{
	u64 cur_cnt = 0;
	asm volatile ("mrs %0, cntpct_el0":"=r" (cur_cnt));
	return (cur_cnt - cntp_init) * NS_IN_US / tick_per_us;
}

u64 plat_get_current_tick(void)
{
	u64 cur_cnt = 0;
	asm volatile ("mrs %0, cntpct_el0":"=r" (cur_cnt));
	return cur_cnt;
}
