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

#ifndef SCHED_SCHED_H
#define SCHED_SCHED_H

#include <sched/context.h>
#include <common/list.h>
#include <machine.h>

/* BUDGET represents the number of TICKs */
#define DEFAULT_BUDGET 1
/*
 * The time interval of one tick in ms
 * (by default, trigger one tick per 10 ms)
 */
#define TICK_MS 10

/* Priority */
#define MAX_PRIO  255
#define MIN_PRIO  1
#define PRIO_NUM  (MAX_PRIO + 1)
#define IDLE_PRIO 0
#ifndef CHCORE_OPENTRUSTEE
#define DEFAULT_PRIO MIN_PRIO
#else /* CHCORE_OPENTRUSTEE */
#define DEFAULT_PRIO 10
#endif /* CHCORE_OPENTRUSTEE */
/* No CPU affinity */
#define NO_AFF (-1)

enum thread_state {
        TS_INIT = 0,
        TS_READY,
        TS_RUNNING,
        TS_WAITING, /* Passive thread wating on register/connection */
        TS_BLOCKING, /* Blocking on notifc or etc */
};

enum kernel_stack_state { KS_FREE = 0, KS_LOCKED };

enum thread_exit_state { TE_RUNNING = 0, TE_EXITING, TE_EXITED };

enum thread_type {
        /*
         * Kernel-level threads
         * 1. Without FPU states
         * 2. Won't swap TLS
         */
        TYPE_IDLE = 0, /* IDLE thread dose not have stack, pause cpu */
        TYPE_KERNEL = 1, /* KERNEL thread has stack */

        /*
         * User-level threads
         * Should be larger than TYPE_KERNEL!
         */
        TYPE_USER = 2,
        TYPE_SHADOW = 3, /* SHADOW thread is used to achieve migrate IPC */
        TYPE_REGISTER = 4, /* Use as the IPC register callback threads */
        TYPE_TRACEE = 5, /* Traced thread for gdb server */
        TYPE_TESTS = 6 /* TESTS thread is used by kernel tests */
};

/* Struct thread declaraion */
struct thread;

struct sched_ops {
        int (*sched_init)(void);
        int (*sched)(void);
        int (*sched_periodic)(void);
        int (*sched_enqueue)(struct thread *thread);
        int (*sched_dequeue)(struct thread *thread);
        /* Debug tools */
        void (*sched_top)(void);
};

/* Provided Scheduling Policies */
extern struct sched_ops pbrr; /* Priority Based Round Robin */
extern struct sched_ops pbfifo; /* Priority Based FIFO */
extern struct sched_ops rr; /* Simple Round Robin */

/* Chosen Scheduling Policies */
extern struct sched_ops *cur_sched_ops;

/* Scheduler module local interfaces */
int switch_to_thread(struct thread *target);
int get_cpubind(struct thread *thread);
struct thread *find_runnable_thread(struct list_head *thread_list);

/* Global interfaces */
/* Print the thread information */
void print_thread(struct thread *thread);

/*
 * A common usage pattern:
 *   sched(); // Choose one thread to run (as current_thread)
 *   eret_to_thread(switch_context()); // Switch context between current_thread
 * and previous thread
 */
vaddr_t switch_context(void);
void eret_to_thread(vaddr_t sp);

/* Arch-dependent func declaraions, which are defined in arch/.../sched.c */
void arch_idle_ctx_init(struct thread_ctx *idle_ctx, void (*func)(void));
void arch_switch_context(struct thread *target);

/* Arch-dependent func declaraions, which are defined in assembly files */
extern void idle_thread_routine(void);
extern void __eret_to_thread(unsigned long sp);

/*
 * Direct switch to the target thread (fast path)
 * or put it into the ready queue (slow path)
 */
void sched_to_thread(struct thread *target);
/* Add a mark indicating re-sched is needed on cpuid */
void add_pending_resched(unsigned int cpuid);
/* Wait until the kernel stack of target thread is free */
void wait_for_kernel_stack(struct thread *thread);

int sched_init(struct sched_ops *sched_ops);

static inline int sched(void)
{
        return cur_sched_ops->sched();
}

static inline int sched_periodic(void)
{
        return cur_sched_ops->sched_periodic();
}

static inline int sched_enqueue(struct thread *thread)
{
        return cur_sched_ops->sched_enqueue(thread);
}

static inline int sched_dequeue(struct thread *thread)
{
        return cur_sched_ops->sched_dequeue(thread);
}

/* Syscalls */
void sys_yield(void);
void sys_top(void);
int sys_suspend(unsigned long thread_cap);
int sys_resume(unsigned long thread_cap);

#endif /* SCHED_SCHED_H */
