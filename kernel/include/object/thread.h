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

#ifndef OBJECT_THREAD_H
#define OBJECT_THREAD_H

#include <common/list.h>
#include <mm/vmspace.h>
#include <sched/sched.h>
#include <object/cap_group.h>
#include <arch/machine/smp.h>
#include <ipc/connection.h>
#include <irq/timer.h>
#include <common/debug.h>

/* Per-CPU variable current_thread is only accessed by its owner CPU. */
#define current_thread (current_threads[smp_get_cpu_id()])
#ifdef CHCORE_KERNEL_RT
/* RT (kernel PREEMT): allocate the stack for each thread  */
#define DEFAULT_KERNEL_STACK_SZ (0x1000)
#else
/* No kernel PREEMT: no stack for each thread, instead, using a per-cpu stack */
#define DEFAULT_KERNEL_STACK_SZ (0)
#endif

#define THREAD_ITSELF ((void *)(-1))

struct thread {
        struct list_head node; // link threads in a same cap_group
        struct list_head ready_queue_node; // link threads in a ready queue
        struct list_head notification_queue_node; // link threads in a
                                                  // notification waiting queue
        struct thread_ctx *thread_ctx; // thread control block

        /*
         * prev_thread switch CPU to this_thread
         *
         * When previous thread is the thread itself,
         * prev_thread will be set to THREAD_ITSELF.
         */
        struct thread *prev_thread;

        /*
         * vmspace: virtual memory address space.
         * vmspace is also stored in the 2nd slot of capability
         */
        struct vmspace *vmspace;

        struct cap_group *cap_group;

        /* Record the thread cap for quick thread recycle. */
        cap_t cap;

        /*
         * Only exists for threads in a server process.
         * If not NULL, it points to one of the three config types.
         */
        void *general_ipc_config;

        struct sleep_state sleep_state;

        /* Used for wake other threads in thread_exit */
        int *clear_child_tid;
};

#define thread_set_ts_ready(thread)                              \
        do {                                                     \
                BUG_ON((thread)->thread_ctx->state == TS_READY); \
                (thread)->thread_ctx->state = TS_READY;          \
        } while (0)

#define thread_set_ts_running(thread)                           \
        do {                                                    \
                BUG_ON((thread)->thread_ctx->state == TS_INIT); \
                (thread)->thread_ctx->state = TS_RUNNING;       \
        } while (0)

#define thread_set_ts_blocking(thread)                             \
        do {                                                       \
                BUG_ON((thread)->thread_ctx->state != TS_RUNNING); \
                (thread)->thread_ctx->state = TS_BLOCKING;         \
        } while (0)

#define thread_set_ts_waiting(thread)                                 \
        do {                                                          \
                BUG_ON((thread)->thread_ctx->type != TYPE_REGISTER    \
                       && (thread)->thread_ctx->type != TYPE_SHADOW); \
                BUG_ON((thread)->thread_ctx->state != TS_INIT         \
                       && (thread)->thread_ctx->state != TS_RUNNING); \
                (thread)->thread_ctx->state = TS_WAITING;             \
        } while (0)

#define thread_is_ts_ready(thread) ((thread)->thread_ctx->state == TS_READY)

#define thread_is_ts_running(thread) ((thread)->thread_ctx->state == TS_RUNNING)

#define thread_is_ts_blocking(thread) \
        ((thread)->thread_ctx->state == TS_BLOCKING)

#define thread_is_ts_waiting(thread) ((thread)->thread_ctx->state == TS_WAITING)

#define thread_set_suspend(thread) ((thread)->thread_ctx->is_suspended = true)

#define thread_set_resume(thread) ((thread)->thread_ctx->is_suspended = false)

#define thread_is_suspend(thread) ((thread)->thread_ctx->is_suspended == true)

#define thread_set_exited(thread) \
        ((thread)->thread_ctx->thread_exit_state = TE_EXITED)

#define thread_is_exiting(thread) \
        ((thread)->thread_ctx->thread_exit_state == TE_EXITING)

#define thread_is_exited(thread) \
        ((thread)->thread_ctx->thread_exit_state == TE_EXITED)

extern struct thread *current_threads[PLAT_CPU_NUM];
extern struct thread idle_threads[PLAT_CPU_NUM];

/* Used for creating the root thread */
extern const char binary_procmgr_bin_start;
extern const unsigned long binary_procmgr_bin_size;

void create_root_thread(void);
void switch_thread_vmspace_to(struct thread *);
void thread_deinit(void *thread_ptr);

/* Syscalls */
cap_t sys_create_thread(unsigned long thread_args_p);
void sys_thread_exit(void);
int sys_set_affinity(cap_t thread_cap, s32 aff);
s32 sys_get_affinity(cap_t thread_cap);
int sys_set_prio(cap_t thread_cap, unsigned int prio);
int sys_get_prio(cap_t thread_cap);
int sys_set_tid_address(int *tidptr);

#ifdef CHCORE_OPENTRUSTEE
cap_t ot_sys_get_thread_id(cap_t thread_cap);
int ot_sys_terminate_thread(cap_t thread_cap);
void ot_sys_disable_local_irq(void);
void ot_sys_enable_local_irq(void);
#endif /* CHCORE_OPENTRUSTEE */

#endif /* OBJECT_THREAD_H */
