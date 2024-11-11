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

#include <sched/sched.h>
#include <arch/machine/registers.h>
#include <object/thread.h>
#include <common/vars.h>
#include <mm/kmalloc.h>
#include <lib/printk.h>

void arch_idle_ctx_init(struct thread_ctx *idle_ctx, void (*func)(void))
{
	/* Initialize to run the function `idle_thread_routine` */
	int i = 0;
	arch_exec_ctx_t *ec = &(idle_ctx->ec);

	/* X0-X30 all zero */
	for (i = 0; i < REG_NUM; i++)
		ec->reg[i] = 0;
	/* SPSR_EL1 => Exit to EL1 */
	ec->reg[SPSR_EL1] = SPSR_EL1_KERNEL;
	/* ELR_EL1 => Next PC */
	ec->reg[ELR_EL1] = (u64) func;
}

void arch_switch_context(struct thread *target)
{
	struct per_cpu_info *info;

	info = get_per_cpu_info();

	/* Set the `cur_exec_ctx` in the per_cpu info. */
	info->cur_exec_ctx = (u64)target->thread_ctx;
}
