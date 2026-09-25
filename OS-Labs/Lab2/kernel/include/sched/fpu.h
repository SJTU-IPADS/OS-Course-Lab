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

#ifndef SCHED_FPU_H
#define SCHED_FPU_H

#include <object/thread.h>
#include <sched/sched.h>

#define EAGER_FPU_MODE 0
#define LAZY_FPU_MODE  1

/* RISC-V only supports eager mode. */
#ifdef CHCORE_ARCH_RISCV64
#define FPU_SAVING_MODE EAGER_FPU_MODE
#else
#define FPU_SAVING_MODE LAZY_FPU_MODE
#endif

void arch_init_thread_fpu(struct thread_ctx *ctx);

void save_fpu_state(struct thread *thread);
void restore_fpu_state(struct thread *thread);

#if FPU_SAVING_MODE == LAZY_FPU_MODE
void disable_fpu_usage(void);
void enable_fpu_usage(void);

/* Used when hanlding FPU traps */
void change_fpu_owner(void);
/* Used when migrating a thread from local CPU to other CPUs */
void save_and_release_fpu(struct thread *thread);
#endif

#endif /* SCHED_FPU_H */