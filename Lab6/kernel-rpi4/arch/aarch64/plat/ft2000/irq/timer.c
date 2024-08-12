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
#include <lib/printk.h>
#include <common/types.h>
#include <common/kprint.h>
#include <arch/machine/smp.h>
#include <sched/sched.h>

u64 cntp_tval;
u64 tick_per_us;
u64 cntp_init;
u64 cntp_freq;

void plat_timer_init(void)
{
	u64 timer_ctl = 0x1;

	/* Get the frequency and init count */
	asm volatile ("mrs %0, cntpct_el0":"=r" (cntp_init));
	kdebug("timer init cntpct_el0 = %lu\n", cntp_init);
	asm volatile ("mrs %0, cntfrq_el0":"=r" (cntp_freq));
	kdebug("timer init cntfrq_el0 = %lu\n", cntp_freq);

	/* Calculate the tv */
	cntp_tval = (cntp_freq * TICK_MS / 1000);
	tick_per_us = cntp_freq / 1000 / 1000;
	// kinfo("CPU freq %lu, set timer %lu\n", cntp_freq, cntp_tval);

	/* Set the timervalue so that we will get a timer interrupt */
	asm volatile ("msr cntp_tval_el0, %0"::"r" (cntp_tval));

	/* Set the control register */
	asm volatile ("msr cntp_ctl_el0, %0"::"r" (timer_ctl));
}

void plat_set_next_timer(u64 tick_delta)
{
	asm volatile ("msr cntp_tval_el0, %0"::"r" (tick_delta));
}

void plat_handle_timer_irq(u64 tick_delta)
{
	/* Set the timervalue */
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
