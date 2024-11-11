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
#include <common/kprint.h>
#include <machine.h>
#include <mm/kmalloc.h>
#include <object/thread.h>

/*
 * RR policy also has idle threads.
 * When no active user threads in ready queue,
 * we will choose the idle thread to execute.
 * Idle thread will **NOT** be in the RQ.
 */

/* Metadata for ready queue */
struct queue_meta {
        struct list_head queue_head;
        unsigned int queue_len;
        struct lock queue_lock;
        char pad[pad_to_cache_line(sizeof(unsigned int)
                                   + sizeof(struct list_head)
                                   + sizeof(struct lock))];
};

/*
 * All the variables (except struct sched_ops rr) and functions are static.
 * We omit the modifier due to they are used in kernel tests.
 */

/* rr_ready_queue: Per-CPU ready queue for ready tasks. */
struct queue_meta rr_ready_queue_meta[PLAT_CPU_NUM];

int __rr_sched_enqueue(struct thread *thread, int cpuid)
{
        if (thread->thread_ctx->type == TYPE_IDLE) {
                return 0;
        }

        /* Already in the ready queue */
        if (thread_is_ts_ready(thread)) {
                return -EINVAL;
        }

        thread->thread_ctx->cpuid = cpuid;
        thread_set_ts_ready(thread);
        obj_ref(thread);
        list_append(&(thread->ready_queue_node),
                    &(rr_ready_queue_meta[cpuid].queue_head));
        rr_ready_queue_meta[cpuid].queue_len++;

        return 0;
}

/* The config can be tuned. */
#define LOADBALANCE_THRESHOLD 5
#define MIGRATE_THRESHOLD     5

/* A simple load balance when enqueue threads */
unsigned int rr_sched_choose_cpu(void)
{
        unsigned int i, cpuid, min_rr_len, local_cpuid, queue_len;

        local_cpuid = smp_get_cpu_id();
        min_rr_len = rr_ready_queue_meta[local_cpuid].queue_len;

        if (min_rr_len <= LOADBALANCE_THRESHOLD) {
                return local_cpuid;
        }

        /* Find the cpu with the shortest ready queue */
        cpuid = local_cpuid;
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                if (i == local_cpuid) {
                        continue;
                }

                queue_len =
                        rr_ready_queue_meta[i].queue_len + MIGRATE_THRESHOLD;
                if (queue_len < min_rr_len) {
                        min_rr_len = queue_len;
                        cpuid = i;
                }
        }

        return cpuid;
}

/*
 * Sched_enqueue
 * Put `thread` at the end of ready queue of assigned `affinity` and `prio`.
 * If affinity = NO_AFF, assign the core to the current cpu.
 * If the thread is IDEL thread, do nothing!
 */
int rr_sched_enqueue(struct thread *thread)
{
        BUG_ON(!thread);
        BUG_ON(!thread->thread_ctx);
        if (thread->thread_ctx->type == TYPE_IDLE)
                return 0;

        int cpubind = 0;
        unsigned int cpuid = 0;
        int ret = 0;

        cpubind = get_cpubind(thread);
        cpuid = cpubind == NO_AFF ? rr_sched_choose_cpu() : cpubind;

        if (unlikely(thread->thread_ctx->sc->prio > MAX_PRIO))
                return -EINVAL;

        if (unlikely(cpuid >= PLAT_CPU_NUM)) {
                return -EINVAL;
        }

        lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        ret = __rr_sched_enqueue(thread, cpuid);
        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
        return ret;
}

/* dequeue w/o lock */
int __rr_sched_dequeue(struct thread *thread)
{
        /* IDLE thread will **not** be in any ready queue */
        BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

        if (!thread_is_ts_ready(thread)) {
                kwarn("%s: thread state is %d\n",
                      __func__,
                      thread->thread_ctx->state);
                return -EINVAL;
        }

        list_del(&(thread->ready_queue_node));
        rr_ready_queue_meta[thread->thread_ctx->cpuid].queue_len--;
        obj_put(thread);
        return 0;
}

/*
 * remove @thread from its current residual ready queue
 */
int rr_sched_dequeue(struct thread *thread)
{
        BUG_ON(!thread);
        BUG_ON(!thread->thread_ctx);
        /* IDLE thread will **not** be in any ready queue */
        BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

        unsigned int cpuid = 0;
        int ret = 0;

        cpuid = thread->thread_ctx->cpuid;
        lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        ret = __rr_sched_dequeue(thread);
        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
        return ret;
}

/*
 * Choose an appropriate thread and dequeue from ready queue
 */
struct thread *rr_sched_choose_thread(void)
{
        unsigned int cpuid = smp_get_cpu_id();
        struct thread *thread = NULL;

        if (!list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        again:
                if (list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;
                }
                /*
                 * When the thread is just moved from another cpu and
                 * the kernel stack is used by the origina core, try
                 * to find another thread.
                 */
                if (!(thread = find_runnable_thread(
                              &(rr_ready_queue_meta[cpuid].queue_head)))) {
                        unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                        goto out;
                }

                BUG_ON(__rr_sched_dequeue(thread));
                if (thread_is_exiting(thread) || thread_is_exited(thread)) {
                        /* Thread need to exit. Set the state to TE_EXITED */
                        thread_set_exited(thread);
                        goto again;
                }
                unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
                return thread;
        }
out:
        return &idle_threads[cpuid];
}

static inline void rr_sched_refill_budget(struct thread *target,
                                          unsigned int budget)
{
        target->thread_ctx->sc->budget = budget;
}

/*
 * Schedule a thread to execute.
 * current_thread can be NULL, or the state is TS_RUNNING or
 * TS_WAITING/TS_BLOCKING. This function will suspend current running thread, if
 * any, and schedule another thread from
 * `(rr_ready_queue_meta[cpuid].queue_head)`.
 * ***the following text might be outdated***
 * 1. Choose an appropriate thread through calling *chooseThread* (Simple
 * Priority-Based Policy)
 * 2. Update current running thread and left the caller to restore the executing
 * context
 */
int rr_sched(void)
{
        /* WITH IRQ Disabled */
        struct thread *old = current_thread;
        struct thread *new = 0;

        if (old) {
                BUG_ON(!old->thread_ctx);

                /* old thread may pass its scheduling context to others. */
                if (old->thread_ctx->type != TYPE_SHADOW
                    && old->thread_ctx->type != TYPE_REGISTER) {
                        BUG_ON(!old->thread_ctx->sc);
                }

                /* Set TE_EXITING after check won't cause any trouble, the
                 * thread will be recycle afterwards. Just a fast path. */
                /* Check whether the thread is going to exit */
                if (thread_is_exiting(old)) {
                        /* Set the state to TE_EXITED */
                        thread_set_exited(old);
                }

                /* check old state */
                if (!thread_is_exited(old)) {
                        if (thread_is_ts_running(old)) {
                                /* A thread without SC should not be TS_RUNNING.
                                 */
                                BUG_ON(!old->thread_ctx->sc);
                                if (old->thread_ctx->sc->budget != 0
                                    && !thread_is_suspend(old)) {
                                        switch_to_thread(old);
                                        return 0; /* no schedule needed */
                                }
                                rr_sched_refill_budget(old, DEFAULT_BUDGET);
                                BUG_ON(rr_sched_enqueue(old) != 0);
                        } else if (!thread_is_ts_blocking(old)
                                   && !thread_is_ts_waiting(old)) {
                                kinfo("thread state: %d\n",
                                      old->thread_ctx->state);
                                BUG_ON(1);
                        }
                }
        }

        BUG_ON(!(new = rr_sched_choose_thread()));
        switch_to_thread(new);

        return 0;
}

int rr_sched_init(void)
{
        int i = 0;

        /* Initialize global variables */
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                init_list_head(&(rr_ready_queue_meta[i].queue_head));
                lock_init(&(rr_ready_queue_meta[i].queue_lock));
                rr_ready_queue_meta[i].queue_len = 0;
        }

        return 0;
}

void rr_top(void)
{
        unsigned int cpuid;
        struct thread *thread;
#define MAX_CAP_GROUP_BUF 128
        struct cap_group *cap_group_buf[MAX_CAP_GROUP_BUF] = {0};
        unsigned int cap_group_num = 0;
        int i = 0;

        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
                lock(&(rr_ready_queue_meta[cpuid].queue_lock));
        }

        printk("\n*****CPU RQ Info*****\n");
        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
                printk("== CPU %d RQ LEN %lu==\n",
                       cpuid,
                       rr_ready_queue_meta[cpuid].queue_len);
                thread = current_threads[cpuid];
                if (thread != NULL) {
                        for (i = 0; i < cap_group_num; i++)
                                if (thread->cap_group == cap_group_buf[i])
                                        break;
                        if (i == cap_group_num
                            && cap_group_num < MAX_CAP_GROUP_BUF) {
                                cap_group_buf[cap_group_num] =
                                        thread->cap_group;
                                cap_group_num++;
                        }
                        printk("Current ");
                        print_thread(thread);
                }
                if (!list_empty(&(rr_ready_queue_meta[cpuid].queue_head))) {
                        for_each_in_list (
                                thread,
                                struct thread,
                                ready_queue_node,
                                &(rr_ready_queue_meta[cpuid].queue_head)) {
                                for (i = 0; i < cap_group_num; i++)
                                        if (thread->cap_group
                                            == cap_group_buf[i])
                                                break;
                                if (i == cap_group_num
                                    && cap_group_num < MAX_CAP_GROUP_BUF) {
                                        cap_group_buf[cap_group_num] =
                                                thread->cap_group;
                                        cap_group_num++;
                                }
                                print_thread(thread);
                        }
                }
                printk("\n");
        }

        printk("\n*****CAP GROUP Info*****\n");
        for (i = 0; i < cap_group_num; i++) {
                printk("== CAP GROUP:%s ==\n",
                       cap_group_buf[i]->cap_group_name);
                for_each_in_list (thread,
                                  struct thread,
                                  node,
                                  &(cap_group_buf[i]->thread_list)) {
                        print_thread(thread);
                }
                printk("\n");
        }
        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
                unlock(&(rr_ready_queue_meta[cpuid].queue_lock));
        }
}

struct sched_ops rr = {.sched_init = rr_sched_init,
                       .sched = rr_sched,
                       .sched_periodic = rr_sched,
                       .sched_enqueue = rr_sched_enqueue,
                       .sched_dequeue = rr_sched_dequeue,
                       .sched_top = rr_top};
