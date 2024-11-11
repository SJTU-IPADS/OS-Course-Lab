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

// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2015 Google Inc. All rights reserved
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <object/thread.h>
#include <sched/context.h>
#include <sched/sched.h>
#include <mm/kmalloc.h>
#include <arch/sync.h>

struct aarch64_fpu_area {
	/* 32 fpu registers and each has 16 bytes */
	u8 fpu[32*16];

	/* fp control reg and state reg */
	u32 fpcr;
	u32 fpsr;
};

void arch_init_thread_fpu(struct thread_ctx *ctx)
{
	struct aarch64_fpu_area *buf;

	/*
	 * In case of data leakage, clear this area (use kzalloc instead
	 * of kmalloc).
	 */
	ctx->fpu_state = kzalloc(sizeof(struct aarch64_fpu_area));
	ctx->is_fpu_owner = -1;
	buf = (struct aarch64_fpu_area *)ctx->fpu_state;

	buf->fpcr = 0;
	buf->fpsr = 0;
}

void arch_free_thread_fpu(struct thread_ctx *ctx)
{
	kfree(ctx->fpu_state);
}

void save_fpu_state(struct thread *thread)
{
	if (likely ((thread)
		    && (thread->thread_ctx->type > TYPE_KERNEL))) {

		struct aarch64_fpu_area *buf;
		u64 fpcr, fpsr;


		buf = (struct aarch64_fpu_area *)thread->thread_ctx->fpu_state;


		__asm__ volatile("stp     q0, q1, [%0, #(0 * 32)]\n"
				 "stp     q2, q3, [%0, #(1 * 32)]\n"
				 "stp     q4, q5, [%0, #(2 * 32)]\n"
				 "stp     q6, q7, [%0, #(3 * 32)]\n"
				 "stp     q8, q9, [%0, #(4 * 32)]\n"
				 "stp     q10, q11, [%0, #(5 * 32)]\n"
				 "stp     q12, q13, [%0, #(6 * 32)]\n"
				 "stp     q14, q15, [%0, #(7 * 32)]\n"
				 "stp     q16, q17, [%0, #(8 * 32)]\n"
				 "stp     q18, q19, [%0, #(9 * 32)]\n"
				 "stp     q20, q21, [%0, #(10 * 32)]\n"
				 "stp     q22, q23, [%0, #(11 * 32)]\n"
				 "stp     q24, q25, [%0, #(12 * 32)]\n"
				 "stp     q26, q27, [%0, #(13 * 32)]\n"
				 "stp     q28, q29, [%0, #(14 * 32)]\n"
				 "stp     q30, q31, [%0, #(15 * 32)]\n"
				 :
				 :"r"(buf->fpu)
				 :"memory"
				);

		/*
		 * These are 32-bit values,
		 * but the msr instruction always uses
		 * a 64-bit destination register.
		 */
		__asm__("mrs %0, fpcr\n" : "=r"(fpcr));
		__asm__("mrs %0, fpsr\n" : "=r"(fpsr));

		buf->fpcr = (u32)fpcr;
		buf->fpsr = (u32)fpsr;
	}
}

void restore_fpu_state(struct thread *thread)
{
	if (likely ((thread)
		    && (thread->thread_ctx->type > TYPE_KERNEL))) {

		struct aarch64_fpu_area *buf;

		buf = (struct aarch64_fpu_area *)thread->thread_ctx->fpu_state;

		__asm__ volatile("ldp     q0, q1, [%0, #(0 * 32)]\n"
				 "ldp     q2, q3, [%0, #(1 * 32)]\n"
				 "ldp     q4, q5, [%0, #(2 * 32)]\n"
				 "ldp     q6, q7, [%0, #(3 * 32)]\n"
				 "ldp     q8, q9, [%0, #(4 * 32)]\n"
				 "ldp     q10, q11, [%0, #(5 * 32)]\n"
				 "ldp     q12, q13, [%0, #(6 * 32)]\n"
				 "ldp     q14, q15, [%0, #(7 * 32)]\n"
				 "ldp     q16, q17, [%0, #(8 * 32)]\n"
				 "ldp     q18, q19, [%0, #(9 * 32)]\n"
				 "ldp     q20, q21, [%0, #(10 * 32)]\n"
				 "ldp     q22, q23, [%0, #(11 * 32)]\n"
				 "ldp     q24, q25, [%0, #(12 * 32)]\n"
				 "ldp     q26, q27, [%0, #(13 * 32)]\n"
				 "ldp     q28, q29, [%0, #(14 * 32)]\n"
				 "ldp     q30, q31, [%0, #(15 * 32)]\n"
				 "msr     fpcr, %1\n"
				 "msr     fpsr, %2\n"
				 :
				 :"r"(buf->fpu),
				 "r"((u64)buf->fpcr),
				 "r"((u64)buf->fpsr)
				);
	}
}

#if FPU_SAVING_MODE == LAZY_FPU_MODE

#define CPACR_EL1_FPEN 20
#define CPACR_EL1_FPEN_MASK 0b11

void disable_fpu_usage(void)
{
	struct per_cpu_info *info;
	u32 cpacr = 0;

	info = get_per_cpu_info();

	if (info->fpu_disable == 0) {
		/* Get current cpacr value */
		asm volatile("mrs %0, cpacr_el1":"=r"(cpacr)::"memory");
		/* Disable using FPU */
		cpacr &= ~(CPACR_EL1_FPEN_MASK << CPACR_EL1_FPEN);
		/* Allow EL1 to use */
		cpacr |= (1 << CPACR_EL1_FPEN);
		asm volatile("msr cpacr_el1, %0"::"r"(cpacr):"memory");

		info->fpu_disable = 1;
	}
}

void enable_fpu_usage(void)
{
	struct per_cpu_info *info;
	u32 cpacr = 0;

	/* Get current cpacr value */
	asm volatile("mrs %0, cpacr_el1":"=r"(cpacr)::"memory");
	/* Enable using FPU */
	cpacr |= (CPACR_EL1_FPEN_MASK << CPACR_EL1_FPEN);
	asm volatile("msr cpacr_el1, %0"::"r"(cpacr):"memory");

	info = get_per_cpu_info();
	info->fpu_disable = 0;
}

/* This function is used as the hander for FPU traps */
void change_fpu_owner(void)
{
	struct per_cpu_info *info;
	struct thread *fpu_owner;
	u32 cpuid;

	enable_fpu_usage();

	cpuid = smp_get_cpu_id();
	lock(&fpu_owner_locks[cpuid]);

	/* Get the current fpu_owner (per CPU) */
	info = get_per_cpu_info();
	fpu_owner = info->fpu_owner;

	/* A (fpu_owner) -> B (no using FPU) -> A */
	if (fpu_owner == current_thread) {
		unlock(&fpu_owner_locks[cpuid]);
		return;
	}

	/* Save the fpu states for the current owner */
	if (fpu_owner) {
		save_fpu_state(fpu_owner);
		/*
		 * A barrier to make sure that fpu state has been saved
		 * before is_fpu_owner has been set to -1.
		 */
		smp_mb();
		fpu_owner->thread_ctx->is_fpu_owner = -1;
	}

	/* Set current_thread as the new owner */
	info->fpu_owner = current_thread;
	unlock(&fpu_owner_locks[cpuid]);

	restore_fpu_state(current_thread);
	current_thread->thread_ctx->is_fpu_owner = cpuid;
}

/*
 * This function is used by the scheduler
 * when it migrates @thread to another CPU.
 *
 * No need to acquire the fpu_owner_lock because
 * @thread is an active thread.
 */
void save_and_release_fpu(struct thread *thread)
{
	struct per_cpu_info *info;
	struct thread *fpu_owner;

	BUG_ON(thread->thread_ctx->thread_exit_state == TE_EXITED);

	/* Get the current fpu_owner (per CPU) */
	info = get_per_cpu_info();
	fpu_owner = info->fpu_owner;

	if (fpu_owner == thread) {
		/*
		 * On aarch64, we always enable OS (EL1) to operate FPU.
		 * Thus, there is no need to invoke enable_fpu_usage before
		 * save_fpu_state as what does on x86_64.
		 */
		save_fpu_state(thread);
		thread->thread_ctx->is_fpu_owner = -1;
		info->fpu_owner = NULL;
		disable_fpu_usage();
	}
}

#endif
