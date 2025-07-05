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

#ifndef SCHED_CONTEXT_H
#define SCHED_CONTEXT_H

#include <arch/sched/arch_sched.h>
#include <common/lock.h>
#include <common/macro.h>
#include <common/types.h>
#include <machine.h>

typedef struct sched_context {
        unsigned int budget;
        unsigned int prio;
} sched_ctx_t;

/* Thread context */
struct thread_ctx {
        /* Arch-dependent */
        /* Executing Context */
        arch_exec_ctx_t ec;
        /* FPU States */
        void *fpu_state;
        /* TLS Related States */
        unsigned long tls_base_reg[TLS_REG_NUM];

        /* Arch-independent */
        /* Is FPU owner on some CPU: -1 means No; other means CPU ID */
        int is_fpu_owner;
        /* Scheduling Context */
        sched_ctx_t *sc;
        /* Thread Type */
        unsigned int type;
        /* Thread state (can not be modified by other cores) */
        unsigned int state;
        /* Whether a thread is being suspended */
        bool is_suspended;
        /* SMP Affinity */
        int affinity;
        /* Current Assigned CPU */
        unsigned int cpuid;
        /* Thread kernel stack state */
        volatile unsigned int kernel_stack_state;
        /* Thread exit state */
        volatile unsigned int thread_exit_state;
} __attribute__((aligned(CACHELINE_SZ)));

struct thread;

extern struct lock fpu_owner_locks[PLAT_CPU_NUM];

struct thread_ctx *create_thread_ctx(unsigned int type);
void destroy_thread_ctx(struct thread *thread);
void init_thread_ctx(struct thread *thread, vaddr_t stack, vaddr_t func,
                     unsigned int prio, unsigned int type, int aff);

/* Arch-dependent */
void arch_set_thread_stack(struct thread *thread, vaddr_t stack);
void arch_set_thread_return(struct thread *thread, unsigned long ret);
vaddr_t arch_get_thread_stack(struct thread *thread);
void arch_set_thread_next_ip(struct thread *thread, vaddr_t ip);
vaddr_t arch_get_thread_next_ip(struct thread *thread);
void arch_set_thread_arg0(struct thread *thread, unsigned long arg);
void arch_set_thread_arg1(struct thread *thread, unsigned long arg);
void arch_set_thread_arg2(struct thread *thread, unsigned long arg);
void arch_set_thread_arg3(struct thread *thread, unsigned long arg);
void arch_set_thread_tls(struct thread *thread, unsigned long tls);
void set_thread_arch_spec_state(struct thread *thread);
void set_thread_arch_spec_state_ipc(struct thread *thread);
void switch_tls_info(struct thread *from, struct thread *to);

#endif /* SCHED_CONTEXT_H */