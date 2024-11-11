/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <machine.h>
#include <sched/sched.h>
#include <mm/kmalloc.h>
#include <common/util.h>
#include <common/kprint.h>
#include <object/thread.h>

/* Extern functions */
void arch_init_thread_fpu(struct thread_ctx *ctx);
void arch_free_thread_fpu(struct thread_ctx *ctx);

/* Each CPU has one lock */
struct lock fpu_owner_locks[PLAT_CPU_NUM];

void init_fpu_owner_locks(void)
{
        int i;

        for (i = 0; i < PLAT_CPU_NUM; ++i)
                lock_init(&fpu_owner_locks[i]);
}

/*
 * When CHCORE_KERNEL_RT is defined:
 *
 * The kernel stack for a thread looks like:
 *
 * High addr
 * ---------- thread_ctx end: kernel_stack_base + DEFAULT_KERNEL_STACK_SZ
 * (0x1000)
 *    ...
 * other fields in the thread_ctx:
 * e.g., fpu_state, tls_state, sched_ctx
 *    ...
 * execution context (e.g., registers)
 * ---------- thread_ctx
 *    ...
 *    ...
 * stack in use for when the thread enters kernel
 *    ...
 *    ...
 * ---------- kernel_stack_base
 * Low addr
 *
 */
struct thread_ctx *create_thread_ctx(unsigned int type)
{
        struct thread_ctx *ctx;
        sched_ctx_t *sc;

#ifdef CHCORE_KERNEL_RT
        void *kernel_stack = kzalloc(DEFAULT_KERNEL_STACK_SZ);
        if (kernel_stack == NULL) {
                kwarn("create_thread_ctx fails due to lack of memory\n");
                return NULL;
        }
        ctx = (struct thread_ctx *)((vaddr_t)kernel_stack
                                    + DEFAULT_KERNEL_STACK_SZ
                                    - sizeof(struct thread_ctx));
#else
        ctx = (struct thread_ctx *)kzalloc(sizeof(struct thread_ctx));
        if (ctx == NULL) {
                kwarn("create_thread_ctx fails due to lack of memory\n");
                return NULL;
        }
#endif

        arch_init_thread_fpu(ctx);

        /* Threads whose types are SHADOW or REGISTER don't have scheduling
         * context. */
        if (type == TYPE_SHADOW || type == TYPE_REGISTER) {
                ctx->sc = NULL;
        } else {
                /* Allocate a scheduling context for threads of other types */
                sc = kzalloc(sizeof(sched_ctx_t));
                if (sc == NULL) {
                        kwarn("create_thread_ctx fails due to lack of memory\n");
#ifdef CHCORE_KERNEL_RT
                        kfree(kernel_stack);
#else
                        kfree(ctx);
#endif
                        return NULL;
                }
                ctx->sc = sc;
        }

        return ctx;
}

void destroy_thread_ctx(struct thread *thread)
{
        unsigned int cpuid;

        BUG_ON(!thread->thread_ctx);

        if (thread->thread_ctx->is_fpu_owner >= 0) {
                cpuid = thread->thread_ctx->cpuid;
                lock(&fpu_owner_locks[cpuid]);
                BUG_ON(cpu_info[cpuid].fpu_owner != thread);
                cpu_info[cpuid].fpu_owner = NULL;
                unlock(&fpu_owner_locks[cpuid]);
        }
        arch_free_thread_fpu(thread->thread_ctx);

        /* Register or shadow threads do not have scheduling contexts */
        if (thread->thread_ctx->type != TYPE_SHADOW
            && thread->thread_ctx->type != TYPE_REGISTER) {
                BUG_ON(!thread->thread_ctx->sc);
                kfree(thread->thread_ctx->sc);
        }

        if (thread->thread_ctx->type == TYPE_SHADOW && thread_is_exited(thread)
            && thread->thread_ctx->sc != NULL) {
                kfree(thread->thread_ctx->sc);
        }

#ifdef CHCORE_KERNEL_RT
        void *kernel_stack;

        kernel_stack = (void *)thread->thread_ctx - DEFAULT_KERNEL_STACK_SZ
                       + sizeof(struct thread_ctx);
        kfree(kernel_stack);
#else
        kfree(thread->thread_ctx);
#endif
}
