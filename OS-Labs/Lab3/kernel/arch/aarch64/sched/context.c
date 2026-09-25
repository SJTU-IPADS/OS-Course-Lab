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

#include <object/thread.h>
#include <sched/sched.h>
#include <arch/machine/registers.h>
#include <arch/machine/smp.h>
#include <mm/kmalloc.h>

void init_thread_ctx(struct thread *thread, vaddr_t stack, vaddr_t func,
                     u32 prio, u32 type, s32 aff)
{
        /* Fill the context of the thread */

        /* LAB 3 TODO BEGIN */
        /* SP_EL0, ELR_EL1, SPSR_EL1*/

        /* LAB 3 TODO END */

        /* Set the state of the thread */
        thread->thread_ctx->state = TS_INIT;

        /* Set thread type */
        thread->thread_ctx->type = type;

        /* Set the cpuid and affinity */
        thread->thread_ctx->affinity = aff;

        /* Set the budget and priority of the thread */
        if (thread->thread_ctx->sc != NULL) {
                thread->thread_ctx->sc->prio = prio;
                thread->thread_ctx->sc->budget = DEFAULT_BUDGET;
        }

        thread->thread_ctx->kernel_stack_state = KS_FREE;
        /* Set exiting state */
        thread->thread_ctx->thread_exit_state = TE_RUNNING;
        thread->thread_ctx->is_suspended = false;
}

vaddr_t arch_get_thread_stack(struct thread *thread)
{
        return thread->thread_ctx->ec.reg[SP_EL0];
}

void arch_set_thread_stack(struct thread *thread, vaddr_t stack)
{
        thread->thread_ctx->ec.reg[SP_EL0] = stack;
}

void arch_set_thread_return(struct thread *thread, unsigned long ret)
{
        thread->thread_ctx->ec.reg[X0] = ret;
}

void arch_set_thread_next_ip(struct thread *thread, vaddr_t ip)
{
        /* Currently, we use fault PC to store the next ip */
        /* Only required when we need to change PC */
        /* Maybe update ELR_EL1 directly */
        thread->thread_ctx->ec.reg[ELR_EL1] = ip;
}

u64 arch_get_thread_next_ip(struct thread *thread)
{
        return thread->thread_ctx->ec.reg[ELR_EL1];
}

/* First argument in X0 */
void arch_set_thread_arg0(struct thread *thread, unsigned long arg)
{
        thread->thread_ctx->ec.reg[X0] = arg;
}

/* Second argument in X1 */
void arch_set_thread_arg1(struct thread *thread, unsigned long arg)
{
        thread->thread_ctx->ec.reg[X1] = arg;
}

void arch_set_thread_arg2(struct thread *thread, unsigned long arg)
{
        thread->thread_ctx->ec.reg[X2] = arg;
}

void arch_set_thread_arg3(struct thread *thread, unsigned long arg)
{
        thread->thread_ctx->ec.reg[X3] = arg;
}

void arch_set_thread_tls(struct thread *thread, unsigned long tls)
{
        thread->thread_ctx->tls_base_reg[0] = tls;
}

/* set arch-specific thread state */
void set_thread_arch_spec_state(struct thread *thread)
{
        /* Currently, nothing need to be done in aarch64. */
}

/* set arch-specific thread state for ipc server thread */
void set_thread_arch_spec_state_ipc(struct thread *thread)
{
        /* Currently, nothing need to be done in aarch64. */
}

/*
 * Saving registers related to thread local storage.
 * On aarch64, TPIDR_EL0 is used by convention.
 */
void switch_tls_info(struct thread *from, struct thread *to)
{
        u64 tpidr_el0;

        if (likely((from) && (from->thread_ctx->type > TYPE_KERNEL))) {
                /* Save TPIDR_EL0 for thread from */
                asm volatile("mrs %0, tpidr_el0\n" : "=r"(tpidr_el0));
                from->thread_ctx->tls_base_reg[0] = tpidr_el0;
        }

        if (likely((to) && (to->thread_ctx->type > TYPE_KERNEL))) {
                /* Restore TPIDR_EL0 for thread to */
                tpidr_el0 = to->thread_ctx->tls_base_reg[0];
                asm volatile("msr tpidr_el0, %0\n" ::"r"(tpidr_el0));
        }
}
