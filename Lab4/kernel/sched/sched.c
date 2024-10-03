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

#include <sched/sched.h>
#include <sched/context.h>
#include <sched/fpu.h>
#include <mm/kmalloc.h>
#include <irq/ipi.h>
#include <common/util.h>

/* Scheduler global data */
struct thread *current_threads[PLAT_CPU_NUM];
struct thread idle_threads[PLAT_CPU_NUM];

#if PLAT_CPU_NUM <= 32
unsigned int resched_bitmaps[PLAT_CPU_NUM];
#else /* PLAT_CPU_NUM <= 32 */
#error PLAT_CPU_NUM exceeds support
#endif /* PLAT_CPU_NUM <= 32 */

/* Chosen Scheduling Policies */
struct sched_ops *cur_sched_ops;

/* Scheduler module local interfaces */

/*
 * Set the current_thread to @target.
 * Note: the switch is between current_thread and @target.
 */
int switch_to_thread(struct thread *target)
{
        BUG_ON(!target);
        BUG_ON(!target->thread_ctx);
        BUG_ON(thread_is_exited(target));

        unsigned int cpuid = smp_get_cpu_id();

#ifdef CHCORE_KERNEL_RT
        resched_bitmaps[cpuid] &= ~BIT(cpuid);
#endif

        /* No thread switch happens actually */
        if (target == current_thread) {
                thread_set_ts_running(target);

                /* The previous thread is the thread itself */
                target->prev_thread = THREAD_ITSELF;
                return 0;
        }

        target->thread_ctx->cpuid = cpuid;

        thread_set_ts_running(target);

        /* Record the thread transferring the CPU */
        target->prev_thread = current_thread;

        target->thread_ctx->kernel_stack_state = KS_LOCKED;

        current_thread = target;

        return 0;
}

/*
 * Return the thread's affinity if possible.
 * Otherwise, return its previously running CPU (FPU owner).
 */
int get_cpubind(struct thread *thread)
{
#if FPU_SAVING_MODE == LAZY_FPU_MODE
        int affinity, is_fpu_owner;
        unsigned int local_cpuid;

        local_cpuid = smp_get_cpu_id();
        affinity = thread->thread_ctx->affinity;
        is_fpu_owner = thread->thread_ctx->is_fpu_owner;

        /*
         * If the thread is the FPU owner of current CPU core,
         * save_and_release_fpu() can make it become not a owner.
         * Therefore, it can be migrated.
         *
         * However, if thread is a FPU owner of some other CPU,
         * we cannot directly migrate it.
         */
        if (is_fpu_owner < 0) {
                return affinity;
        } else if (is_fpu_owner == local_cpuid) {
                if (affinity == local_cpuid || affinity == NO_AFF) {
                        return local_cpuid;
                } else {
                        save_and_release_fpu(thread);
                        return affinity;
                }
        } else {
                return is_fpu_owner;
        }
#else
        return thread->thread_ctx->affinity;
#endif
}

/*
 * find_runnable_thread
 * ** Shoule hold a dedicated lock for the thread_list and this function can
 * only be called in the critical section!! ** Only the thread which kernel
 * state is free can be choosed
 */
struct thread *find_runnable_thread(struct list_head *thread_list)
{
        struct thread *thread = NULL;

        /* LAB 4 TODO BEGIN (exercise 3) */
        /* Tip 1: use for_each_in_list to iterate the thread list */
        /*
         * Tip 2: Find the first thread in the ready queue that
         * satisfies (!thread->thread_ctx->is_suspended && 
         * (thread->thread_ctx->kernel_stack_state == KS_FREE
         * || thread == current_thread))
         */

        /* LAB 4 TODO END (exercise 3) */
        return thread;
}

/* Global interfaces */

void print_thread(struct thread *thread)
{
#define TYPE_STR_LEN 20
        char thread_type[][TYPE_STR_LEN] = {
                "IDLE", "KERNEL", "USER", "SHADOW", "REGISTER", "TESTS"};

#define STATE_STR_LEN 20
        char thread_state[][STATE_STR_LEN] = {
                "TS_INIT      ",
                "TS_READY     ",
                "TS_RUNNING   ",
                "TS_WAITING   ",
                "TS_BLOCKING  ",
        };

        printk("Thread %p\tType: %s\tState: %s\tCPU %d\tAFF %d\tSUSPEND %d\t"
               "Budget %d\tPrio: %d\tIP: %p\tCMD: %s\n",
               thread,
               thread_type[thread->thread_ctx->type],
               thread_state[thread->thread_ctx->state],
               thread->thread_ctx->cpuid,
               thread->thread_ctx->affinity,
               thread->thread_ctx->is_suspended,
               /* REGISTER and SHADOW threads may have no sc, so just print -1.
                */
               thread->thread_ctx->sc ? thread->thread_ctx->sc->budget : -1,
               thread->thread_ctx->sc ? thread->thread_ctx->sc->prio : -1,
               arch_get_thread_next_ip(thread),
               thread->cap_group->cap_group_name);
}

/*
 * This function is used after current_thread is set (a new thread needs to be
 * scheduled).
 *
 * Switch context between current_thread and current_thread->prev_thread:
 * including: vmspace, fpu, tls, ...
 *
 * Return the context pointer which should be set to stack pointer register.
 */
vaddr_t switch_context(void)
{
        struct thread *target_thread;
        struct thread_ctx *target_ctx;
        struct thread *prev_thread;

        target_thread = current_thread;
        if (!target_thread || !target_thread->thread_ctx) {
                kwarn("%s no thread_ctx", __func__);
                return 0;
        }

        target_ctx = target_thread->thread_ctx;

        prev_thread = target_thread->prev_thread;
        if (prev_thread == THREAD_ITSELF)
                return (vaddr_t)target_ctx;

#if FPU_SAVING_MODE == EAGER_FPU_MODE
        save_fpu_state(prev_thread);
        restore_fpu_state(target_thread);
#else
        /* FPU_SAVING_MODE == LAZY_FPU_MODE */
        if (target_thread->thread_ctx->type > TYPE_KERNEL)
                disable_fpu_usage();
#endif

        /* Switch the TLS information: save and restore */
        switch_tls_info(prev_thread, target_thread);

#ifndef CHCORE_KERNEL_TEST
        BUG_ON(!target_thread->vmspace);
        /*
         * Recording the CPU the thread runs on: for TLB maintainence.
         * switch_context is always required for running a (new) thread.
         * So, we invoke record_running_cpu here.
         */
        record_history_cpu(target_thread->vmspace, smp_get_cpu_id());
        if ((!prev_thread) || (prev_thread->vmspace != target_thread->vmspace))
                switch_thread_vmspace_to(target_thread);
#else /* CHCORE_KERNEL_TEST */
        /* TYPE_TESTS threads do not have vmspace. */
        if (target_thread->thread_ctx->type != TYPE_TESTS) {
                BUG_ON(!target_thread->vmspace);
                record_history_cpu(target_thread->vmspace, smp_get_cpu_id());
                switch_thread_vmspace_to(target_thread);
        }
#endif /* CHCORE_KERNEL_TEST */

        arch_switch_context(target_thread);

        return (vaddr_t)target_ctx;
}

#ifdef CHCORE_KERNEL_RT
void do_pending_resched(void)
{
        unsigned int cpuid;
        unsigned int local_cpuid = smp_get_cpu_id();
        bool local_cpu_need_resched = false;

        while (resched_bitmaps[local_cpuid]) {
                cpuid = bsr(resched_bitmaps[local_cpuid]);
                resched_bitmaps[local_cpuid] &= ~BIT(cpuid);
                if (cpuid != local_cpuid) {
                        send_ipi(cpuid, IPI_RESCHED);
                } else {
                        local_cpu_need_resched = true;
                }
        }

        if (local_cpu_need_resched) {
                sched();
                eret_to_thread(switch_context());
        }
}
#endif

void finish_switch(void)
{
        struct thread *prev_thread;

        prev_thread = current_thread->prev_thread;
        if ((prev_thread == THREAD_ITSELF) || (prev_thread == NULL))
                return;

        prev_thread->thread_ctx->kernel_stack_state = KS_FREE;

        if (prev_thread->thread_ctx->type == TYPE_SHADOW
            && thread_is_exited(prev_thread)) {
                cap_free(prev_thread->cap_group, prev_thread->cap);
                current_thread->prev_thread = NULL;
                return;
        }

#ifdef CHCORE_KERNEL_RT
        /* If a resched IPI is received during send_ipi(), the local CPU will
         * re-schedule. */
        do_pending_resched();
#endif
}

/*
 * Transfer the CPU control flow to current_thread (sp).
 * The usage: eret_to_thread(switch_context());
 */
void eret_to_thread(vaddr_t sp)
{
#ifndef CHCORE_KERNEL_RT
        finish_switch();
#endif
        /* Defined as an asm func. */
        __eret_to_thread(sp);
}

/*
 * Exposed for (directly) scheduling to one thread (for example, in IPC and
 * Notification).
 * - Fast path: if direct switch is possible, the core scheduler will not
 * involve.
 * - Slow path: otherwise, fallback to the core scheduler (no direct thread
 * switch).
 *
 * Note that this function never return back to the caller.
 */
void sched_to_thread(struct thread *target)
{
        int is_fpu_owner;

        BUG_ON(!thread_is_ts_blocking(target) && !thread_is_ts_waiting(target));

        /* Switch to itself? */
        BUG_ON(target == current_thread);

        /*
         * We need to ensure the target thread kstack is free
         * before switching to it.
         *
         * Otherwise, wrong cases may occur. For example:
         * Time-1: CPU-0: T1 sends ipc to T2 ->
         *                T2 is enqueued and T1 is current thread
         *
         * Time-2: CPU-1: T2 runs and returns to T1 ->
         *                T1's state may be TS_READY.
         *
         * Time-3: CPU-0: sched.c finds old thread (T1)'s state is
         *                TS_READY (triggers BUG_ON).
         *
         * Another example:
         * If T1 want to direct switch to T2.
         * The target (server) thread may be still executing after
         * IPC returns (a very small time window),
         * so we need to wait until its stack is free.
         */
        wait_for_kernel_stack(target);

        /*
         * Since the target thread is waiting or be set to TS_INTER in
         * signal_notific, its is_fpu_owner state will not change
         * or can only be changed to -1.
         */
        is_fpu_owner = target->thread_ctx->is_fpu_owner;

        if ((is_fpu_owner >= 0) && (is_fpu_owner != smp_get_cpu_id())) {
                /*
                 * Slow path: if target thread is fpu_owner of some other CPUs,
                 * local CPU cannot direct switch to it.
                 */

                BUG_ON(sched_enqueue(target));

                sched();
        } else {
                /* Fast path: direct switch to target thread (without
                 * scheduling). */

                BUG_ON(!thread_is_ts_blocking(current_thread)
                       && !thread_is_ts_waiting(current_thread));

                switch_to_thread(target);
        }

        eret_to_thread(switch_context());
}

/* Pending rescheduling will be done when the kernel returns to userspace */
void add_pending_resched(unsigned int cpuid)
{
        BUG_ON(cpuid >= PLAT_CPU_NUM);
        resched_bitmaps[smp_get_cpu_id()] |= BIT(cpuid);
}

void wait_for_kernel_stack(struct thread *thread)
{
        /*
         * Handle IPI tx while waiting to avoid deadlock.
         *
         * Deadlock example:
         * CPU 0: waiting for CPU 1 to release its kernel stack,
         *        cannot receive IPI as it is in kernel mode;
         * CPU 1: waiting for CPU 0 to finish an IPI tx,
         *        cannot release its kernel stack as it is
         *        executing on that stack.
         */
        while (thread->thread_ctx->kernel_stack_state != KS_FREE) {
                handle_ipi();
        }
}

static void init_idle_threads(void)
{
        unsigned int i;
        const char *idle_name = "KERNEL-IDLE";
        unsigned int idle_name_len = strlen(idle_name);
        struct cap_group *idle_cap_group;
        struct vmspace *idle_vmspace;

        /* Create a fake idle cap group to store the name */
        idle_cap_group = kzalloc(sizeof(*idle_cap_group));
        idle_name_len = MIN(idle_name_len, MAX_GROUP_NAME_LEN);

        BUG_ON(!idle_cap_group);

        memcpy(idle_cap_group->cap_group_name, idle_name, idle_name_len);
        init_list_head(&idle_cap_group->thread_list);

        idle_vmspace = create_idle_vmspace();

        for (i = 0; i < PLAT_CPU_NUM; i++) {
                idle_threads[i].thread_ctx = create_thread_ctx(TYPE_IDLE);
                BUG_ON(idle_threads[i].thread_ctx == NULL);

                init_thread_ctx(
                        &idle_threads[i], 0, 0, IDLE_PRIO, TYPE_IDLE, i);
                thread_set_ts_ready(&idle_threads[i]);

                arch_idle_ctx_init(idle_threads[i].thread_ctx,
                                   idle_thread_routine);

                idle_threads[i].cap_group = idle_cap_group;
                idle_threads[i].vmspace = idle_vmspace;
                list_add(&idle_threads[i].node, &idle_cap_group->thread_list);
        }
        kdebug("Create %d idle threads.\n", i);
}

static void init_current_threads(void)
{
        int i;

        for (i = 0; i < PLAT_CPU_NUM; i++) {
                current_threads[i] = NULL;
        }
}

static void init_resched_bitmaps(void)
{
        memset(resched_bitmaps, 0, sizeof(resched_bitmaps));
}

int sched_init(struct sched_ops *sched_ops)
{
        BUG_ON(sched_ops == NULL);

        init_idle_threads();
        init_current_threads();
        init_resched_bitmaps();

        cur_sched_ops = sched_ops;
        cur_sched_ops->sched_init();

        return 0;
}

/* SYSCALL functions */
void sys_yield(void)
{
        current_thread->thread_ctx->sc->budget = 0;
        /* LAB 4 TODO BEGIN (exercise 4) */
        /* Trigger sched */
        /* Note: you should just add a function call (one line of code) */

        /* LAB 4 TODO END (exercise 4) */
        eret_to_thread(switch_context());
}

void sys_top(void)
{
        cur_sched_ops->sched_top();
}

int sys_suspend(unsigned long thread_cap)
{
        struct thread *thread = NULL;

        if (thread_cap != 0) {
                thread = obj_get(current_cap_group, thread_cap, TYPE_THREAD);
                if (thread == NULL)
                        return -ECAPBILITY;
                thread_set_suspend(thread);
                obj_put(thread);
        } else {
                thread_set_suspend(current_thread);
        }

        /* Self-suspend */
        if (thread_cap == 0 || thread == current_thread) {
                arch_set_thread_return(current_thread, 0);
                sched();
                eret_to_thread(switch_context());

                BUG("Should never reach here\n");
        }

        return 0;
}

int sys_resume(unsigned long thread_cap)
{
        struct thread *thread;

        thread = obj_get(current_cap_group, thread_cap, TYPE_THREAD);
        if (thread == NULL)
                return -ECAPBILITY;

        thread_set_resume(thread);
        obj_put(thread);
        return 0;
}
