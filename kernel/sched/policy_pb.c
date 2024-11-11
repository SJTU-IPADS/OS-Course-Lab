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
#include <common/util.h>
#include <common/bitops.h>
#include <common/kprint.h>
#include <object/thread.h>

/*
 * All variables (except struct sched_ops pbrr/pbfifo) and functions are static.
 * We omit some modifiers due to they are used in kernel tests.
 */

/*
 * Priority bitmap.
 * Record the priorities of the ready threads in a bitmap
 * so that we can find the ready thread with the highest priortiy in O(1) time.
 * Single bitmap for small PRIO_NUM and multi-level bitmap for large PRIO_NUM.
 */
#define PRIOS_PER_LEVEL  32
#define PRIO_LEVEL_SHIFT 5
#define PRIO_LEVEL_MASK  0x1f

#if PRIO_NUM <= PRIOS_PER_LEVEL
struct prio_bitmap {
        unsigned int bitmap;
};

static inline void prio_bitmap_init(struct prio_bitmap *bitmap)
{
        bitmap->bitmap = 0;
}

static inline void prio_bitmap_set(struct prio_bitmap *bitmap,
                                   unsigned int prio)
{
        bitmap->bitmap |= BIT(prio);
}

static inline void prio_bitmap_clear(struct prio_bitmap *bitmap,
                                     unsigned int prio)
{
        bitmap->bitmap &= ~BIT(prio);
}

static inline bool prio_bitmap_is_empty(struct prio_bitmap *bitmap)
{
        return bitmap->bitmap == 0;
}

static inline int get_highest_prio(struct prio_bitmap *bitmap)
{
        return bsr(bitmap->bitmap);
}
#elif PRIO_NUM <= PRIOS_PER_LEVEL * PRIOS_PER_LEVEL

struct prio_bitmap {
        unsigned int bitmap_lvl0;
        unsigned int bitmap_lvl1[PRIOS_PER_LEVEL];
};

static inline void prio_bitmap_init(struct prio_bitmap *bitmap)
{
        memset(bitmap, 0, sizeof(*bitmap));
}

static inline void prio_bitmap_set(struct prio_bitmap *bitmap,
                                   unsigned int prio)
{
        unsigned int index_lvl0 = prio >> PRIO_LEVEL_SHIFT;
        unsigned int index_lvl1 = prio & PRIO_LEVEL_MASK;

        BUG_ON(index_lvl0 >= PRIOS_PER_LEVEL);

        bitmap->bitmap_lvl0 |= BIT(index_lvl0);
        bitmap->bitmap_lvl1[index_lvl0] |= BIT(index_lvl1);
}

static inline void prio_bitmap_clear(struct prio_bitmap *bitmap,
                                     unsigned int prio)
{
        unsigned int index_lvl0 = prio >> PRIO_LEVEL_SHIFT;
        unsigned int index_lvl1 = prio & PRIO_LEVEL_MASK;

        BUG_ON(index_lvl0 >= PRIOS_PER_LEVEL);

        bitmap->bitmap_lvl1[index_lvl0] &= ~BIT(index_lvl1);
        if (bitmap->bitmap_lvl1[index_lvl0] == 0) {
                bitmap->bitmap_lvl0 &= ~BIT(index_lvl0);
        }
}

static inline bool prio_bitmap_is_empty(struct prio_bitmap *bitmap)
{
        return bitmap->bitmap_lvl0 == 0;
}

static inline unsigned int get_highest_prio(struct prio_bitmap *bitmap)
{
        unsigned int index_lvl0;
        unsigned int index_lvl1;

        index_lvl0 = bsr(bitmap->bitmap_lvl0);
        index_lvl1 = bsr(bitmap->bitmap_lvl1[index_lvl0]);

        return (index_lvl0 << PRIO_LEVEL_SHIFT) + index_lvl1;
}
#else
#error PRIO_NUM should not be larger than 1024
#endif

struct pb_ready_queue {
        struct list_head queues[PRIO_NUM];
        struct prio_bitmap bitmap;
        struct lock lock;
};

static struct pb_ready_queue pb_ready_queues[PLAT_CPU_NUM];

static int __pb_sched_enqueue(struct thread *thread, bool enqueue_ahead)
{
        int cpubind;
        unsigned int cpuid, prio;
        struct pb_ready_queue *ready_queue;

        BUG_ON(thread == NULL);
        BUG_ON(thread->thread_ctx == NULL);
        if (thread->thread_ctx->type == TYPE_IDLE) {
                return 0;
        }

        /* Already in a ready queue */
        if (thread_is_ts_ready(thread)) {
                return -EINVAL;
        }

        prio = thread->thread_ctx->sc->prio;
        BUG_ON(prio >= PRIO_NUM);

        cpubind = get_cpubind(thread);
        cpuid = (cpubind == NO_AFF ? smp_get_cpu_id() : cpubind);

        thread->thread_ctx->cpuid = cpuid;
        thread_set_ts_ready(thread);

        ready_queue = &pb_ready_queues[cpuid];
        lock(&ready_queue->lock);
        if (enqueue_ahead)
                list_add(&thread->ready_queue_node, &ready_queue->queues[prio]);
        else
                list_append(&thread->ready_queue_node,
                            &ready_queue->queues[prio]);
        prio_bitmap_set(&ready_queue->bitmap, prio);
        unlock(&ready_queue->lock);

#ifdef CHCORE_KERNEL_TEST
        if (thread->thread_ctx->type != TYPE_TESTS) {
                add_pending_resched(cpuid);
        }
#else
        add_pending_resched(cpuid);
#endif

        return 0;
}

int pb_sched_enqueue(struct thread *thread)
{
        BUG_ON(thread == NULL);
        BUG_ON(thread->thread_ctx == NULL);
        if (thread->thread_ctx->type == TYPE_IDLE)
                return 0;

        return __pb_sched_enqueue(thread, false);
}

static void __pb_sched_dequeue(struct thread *thread)
{
        /* IDLE thread will **not** be in any ready queue */
        BUG_ON(thread->thread_ctx->type == TYPE_IDLE);

        unsigned int cpuid, prio;
        struct pb_ready_queue *ready_queue;

        cpuid = thread->thread_ctx->cpuid;
        prio = thread->thread_ctx->sc->prio;
        ready_queue = &pb_ready_queues[cpuid];

        list_del(&thread->ready_queue_node);
        if (list_empty(&ready_queue->queues[prio])) {
                prio_bitmap_clear(&ready_queue->bitmap, prio);
        }
}

static int pb_sched_dequeue(struct thread *thread)
{
        return -1; // unused
}

static struct thread *__pb_sched_choose_thread(bool enable_fifo)
{
        unsigned int cpuid, highest_prio, new_prio;
        struct thread *thread;
        struct pb_ready_queue *ready_queue;
        bool current_thread_runnable;

        cpuid = smp_get_cpu_id();
        ready_queue = &pb_ready_queues[cpuid];

        thread = current_thread;
        current_thread_runnable =
                (thread != NULL) && thread_is_ts_running(thread)
                && !thread_is_suspend(thread) && !thread_is_exited(thread)
                && (thread->thread_ctx->affinity == NO_AFF
                    || thread->thread_ctx->affinity == cpuid);

        lock(&ready_queue->lock);

retry:
        thread = current_thread;

        if (prio_bitmap_is_empty(&ready_queue->bitmap)) {
                highest_prio = 0;
        } else {
                highest_prio = get_highest_prio(&ready_queue->bitmap);
        }

        /* Check whether we should choose current_thread */
        if (current_thread_runnable
            && (thread->thread_ctx->sc->prio > highest_prio
                || (thread->thread_ctx->sc->prio == highest_prio
                    && ((thread->thread_ctx->sc->budget > 0) || enable_fifo)))) {
                goto out_unlock_ready_queue;
        }

        /*
         * If the thread is just moved from another CPU and
         * the kernel stack is still used by the original CPU,
         * just choose the idle thread.
         *
         * We assume that thread moving between CPUs is rare
         * in realtime system, because users usually set the
         * CPU affinity of the threads.
         *
         * Iterate all possible priorities since the high-priortiy
         * thread may be suspended.
         */
        new_prio = highest_prio;
        thread = NULL;

        while (new_prio > IDLE_PRIO && thread == NULL) {
                thread = find_runnable_thread(&ready_queue->queues[new_prio]);
                new_prio--;
        }

        if (thread == NULL) {
                thread = &idle_threads[cpuid];
                goto out_unlock_ready_queue;
        }

        __pb_sched_dequeue(thread);

        /* If the thread is going to exit, choose another thread. */
        if (thread_is_exiting(thread) || thread_is_exited(thread)) {
                thread_set_exited(thread);
                goto retry;
        }

out_unlock_ready_queue:
        unlock(&ready_queue->lock);
        return thread;
}

static void pb_top(void)
{
        unsigned int cpuid;
        struct thread *thread;
#define MAX_CAP_GROUP_BUF 128
        struct cap_group *cap_group_buf[MAX_CAP_GROUP_BUF] = {0};
        unsigned int cap_group_num = 0;
        int i = 0, j = 0;

        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
                lock(&(pb_ready_queues[cpuid].lock));
        }

        printk("\n*****CPU RQ Info*****\n");
        for (cpuid = 0; cpuid < PLAT_CPU_NUM; cpuid++) {
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
                for (j = MAX_PRIO; j >= MIN_PRIO; j--) {
                        if (!list_empty(&(pb_ready_queues[cpuid].queues[j]))) {
                                for_each_in_list (
                                        thread,
                                        struct thread,
                                        ready_queue_node,
                                        &(pb_ready_queues[cpuid].queues[j])) {
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
                                printk("\n");
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
                unlock(&(pb_ready_queues[cpuid].lock));
        }
}

static int __pb_sched(bool enable_fifo)
{
        struct thread *old, *new;

        old = current_thread;

        /* Check whether the old thread is going to exit */
        if (old != NULL && thread_is_exiting(old)) {
                thread_set_exited(old);
        }

        new = __pb_sched_choose_thread(enable_fifo);
        BUG_ON(new == NULL);

        if (old != NULL && new != old && !thread_is_exited(old)
            && thread_is_ts_running(old)) {
                BUG_ON(__pb_sched_enqueue(old, enable_fifo));
        }

        if (new->thread_ctx->sc->budget == 0) {
                new->thread_ctx->sc->budget = DEFAULT_BUDGET;
        }

        switch_to_thread(new);

        return 0;
}

int pbrr_sched(void)
{
        return __pb_sched(false);
}

int pbfifo_sched(void)
{
        return __pb_sched(true);
}

int pb_sched_init(void)
{
        unsigned int i, j;

        /* Initialize the ready queues */
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                prio_bitmap_init(&pb_ready_queues[i].bitmap);
                lock_init(&pb_ready_queues[i].lock);
                for (j = 0; j < PRIO_NUM; j++) {
                        init_list_head(&pb_ready_queues[i].queues[j]);
                }
        }

        /* Insert the idle threads into the ready queues */
        for (i = 0; i < PLAT_CPU_NUM; i++) {
                pb_sched_enqueue(&idle_threads[i]);
        }

        return 0;
}

struct sched_ops pbrr = {.sched_init = pb_sched_init,
                         .sched = pbrr_sched,
                         .sched_periodic = pbrr_sched,
                         .sched_enqueue = pb_sched_enqueue,
                         .sched_dequeue = pb_sched_dequeue,
                         .sched_top = pb_top};

struct sched_ops pbfifo = {.sched_init = pb_sched_init,
                           .sched = pbrr_sched,
                           .sched_periodic = pbfifo_sched,
                           .sched_enqueue = pb_sched_enqueue,
                           .sched_dequeue = pb_sched_dequeue,
                           .sched_top = pb_top};
