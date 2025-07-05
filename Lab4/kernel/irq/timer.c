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

#include <irq/irq.h>
#include <irq/timer.h>
#include <sched/sched.h>
#include <arch/machine/smp.h>
#include <object/thread.h>
#include <common/kprint.h>
#include <common/list.h>
#include <common/lock.h>
#include <mm/uaccess.h>
#include <sched/context.h>

/* Per-core timer states */
struct time_state {
        /* The tick when the next timer irq will occur */
        u64 next_expire;
        /*
         * Record all sleepers on each core.
         * Threads in sleep_list are sorted by the time to wakeup.
         * It is better to use more efficient data structure (tree)
         */
        struct list_head sleep_list;
        /* Protect per core sleep_list */
        struct lock sleep_list_lock;
};

struct time_state time_states[PLAT_CPU_NUM];

void timer_init(void)
{
        int i;

        if (smp_get_cpu_id() == 0) {
                for (i = 0; i < PLAT_CPU_NUM; i++) {
                        init_list_head(&time_states[i].sleep_list);
                        lock_init(&time_states[i].sleep_list_lock);
                }
        }

        /* Per-core timer init */
        plat_timer_init();
}

/* Should be called when holding sleep_list_lock */
static u64 get_next_tick_delta(void)
{
        u64 waiting_tick, current_tick;
        struct list_head *local_sleep_list;
        struct sleep_state *first_sleeper;

        local_sleep_list = &time_states[smp_get_cpu_id()].sleep_list;

        /* Default tick */
        waiting_tick = TICK_MS * US_IN_MS * tick_per_us;
        if (list_empty(local_sleep_list))
                return waiting_tick;

        current_tick = plat_get_current_tick();
        first_sleeper = list_entry(
                local_sleep_list->next, struct sleep_state, sleep_node);
        /* If a thread will wake up before default tick, update the tick. */
        if (current_tick + waiting_tick > first_sleeper->wakeup_tick)
                waiting_tick =
                        first_sleeper->wakeup_tick > current_tick ?
                                first_sleeper->wakeup_tick - current_tick :
                                0;

        return waiting_tick;
}

static void sleep_timer_cb(struct thread *thread);
void handle_timer_irq(void)
{
        u64 current_tick, tick_delta;
        struct time_state *local_time_state;
        struct list_head *local_sleep_list;
        struct lock *local_sleep_list_lock;
        struct sleep_state *iter = NULL, *tmp = NULL;
        struct thread *wakeup_thread;

        /* Remove the thread to wakeup from sleep list */
        current_tick = plat_get_current_tick();
        local_time_state = &time_states[smp_get_cpu_id()];
        local_sleep_list = &local_time_state->sleep_list;
        local_sleep_list_lock = &local_time_state->sleep_list_lock;

        lock(local_sleep_list_lock);
        for_each_in_list_safe (iter, tmp, sleep_node, local_sleep_list) {
                if (iter->wakeup_tick > current_tick) {
                        break;
                }

                wakeup_thread = container_of(iter, struct thread, sleep_state);

                /*
                 * Grab the thread's queue_lock before operating the
                 * waiting list (sleep_list and the wait_list of
                 * timedout notification.
                 */
                lock(&wakeup_thread->sleep_state.queue_lock);

                list_del(&iter->sleep_node);

                BUG_ON(wakeup_thread->sleep_state.cb == sleep_timer_cb
                       && !thread_is_ts_blocking(wakeup_thread));
                kdebug("wake up t:%p at:%ld\n", wakeup_thread, current_tick);
                BUG_ON(wakeup_thread->sleep_state.cb == NULL);

                wakeup_thread->sleep_state.cb(wakeup_thread);
                wakeup_thread->sleep_state.cb = NULL;

                unlock(&wakeup_thread->sleep_state.queue_lock);
        }

        /* Set when the next timer irq will arrive */
        tick_delta = get_next_tick_delta();
        unlock(local_sleep_list_lock);

        time_states[smp_get_cpu_id()].next_expire = current_tick + tick_delta;
        plat_handle_timer_irq(tick_delta);
        /* LAB 4 TODO BEGIN (exercise 6) */
        /* Decrease the budget of current thread by 1 if current thread is not NULL */
        /* We will call the sched_periodic in the caller handle_irq so no need to call sched() now. */

        /* LAB 4 TODO END (exercise 6) */
}

/*
 * clock_gettime:
 * - now we all clock sources are monotime
 * - the return time is caculated from the system boot
 */
int sys_clock_gettime(clockid_t clock, struct timespec *ts)
{
        struct timespec ts_k;
        u64 mono_ns;
        int r = 0;

        if (!ts)
                return -EINVAL;

        mono_ns = plat_get_mono_time();

        ts_k.tv_sec = mono_ns / NS_IN_S;
        ts_k.tv_nsec = mono_ns % NS_IN_S;

        r = copy_to_user(ts, &ts_k, sizeof(ts_k));
        if (r) {
                r = -EINVAL;
                goto out_fail;
        }

        return 0;
out_fail:
        return r;
}

int enqueue_sleeper(struct thread *thread, const struct timespec *timeout,
                    timer_cb cb)
{
        u64 s, ns, total_us;
        u64 wakeup_tick;
        struct time_state *local_time_state;
        struct list_head *local_sleep_list;
        struct lock *local_sleep_list_lock;
        struct sleep_state *iter;

        s = timeout->tv_sec;
        ns = timeout->tv_nsec;
        total_us = s * US_IN_S + ns / NS_IN_US;

        wakeup_tick = plat_get_current_tick() + total_us * tick_per_us;
        thread->sleep_state.wakeup_tick = wakeup_tick;
        thread->sleep_state.sleep_cpu = smp_get_cpu_id();

        local_time_state = &time_states[smp_get_cpu_id()];
        local_sleep_list = &local_time_state->sleep_list;
        local_sleep_list_lock = &local_time_state->sleep_list_lock;

        lock(local_sleep_list_lock);
        for_each_in_list (
                iter, struct sleep_state, sleep_node, local_sleep_list) {
                if (iter->wakeup_tick > wakeup_tick)
                        break;
        }
        list_append(&thread->sleep_state.sleep_node, &iter->sleep_node);
        thread->sleep_state.cb = cb;

        unlock(local_sleep_list_lock);

        /*
         * If the current sleep need to wake up earlier than when next timer
         * irq occurs, update timer.
         */
        kdebug("next tick:%lld current tick:%lld\n",
               wakeup_tick,
               time_states[smp_get_cpu_id()].next_expire);
        if (time_states[smp_get_cpu_id()].next_expire > wakeup_tick) {
                time_states[smp_get_cpu_id()].next_expire = wakeup_tick;
                plat_set_next_timer(total_us * tick_per_us);
        }

        return 0;
}

/* Returns true if dequeue successfully, false otherwise */
bool try_dequeue_sleeper(struct thread *thread)
{
        struct time_state *target_time_state;
        struct lock *target_sleep_list_lock;
        bool ret = false;

        BUG_ON(thread == NULL);
        target_time_state = &time_states[thread->sleep_state.sleep_cpu];
        target_sleep_list_lock = &target_time_state->sleep_list_lock;

        /*
         * This rountine will be invoked in sys_notify.
         * Use try_lock for preventing dead lock. sys_notify can be retried.
         */
        if (try_lock(target_sleep_list_lock) == 0) {
                BUG_ON(thread->sleep_state.cb == NULL);

                list_del(&thread->sleep_state.sleep_node);
                thread->sleep_state.cb = NULL;
                ret = true;

                unlock(target_sleep_list_lock);
        }

        return ret;
}

static void sleep_timer_cb(struct thread *thread)
{
        BUG_ON(sched_enqueue(thread));
}

int sys_clock_nanosleep(clockid_t clk, int flags, const struct timespec *req,
                        struct timespec *rem)
{
        int ret;
        struct timespec ts_k = {0};

        kdebug("sleep clk:%d flag:%x ", clk, flags);

        if (rem != NULL) {
                rem = NULL;
        }

        ret = copy_from_user(&ts_k, (void *)req, sizeof(ts_k));
        if (ret) {
                return -EFAULT;
        }
        kdebug("sys_clock_nanosleep req:%ld.%ld\n", ts_k.tv_sec, ts_k.tv_nsec);
        /*
         * Note: every operation that inserts/removes one thread into
         * a waiting queue (no mater sleep or wait_notific)
         * should grab the thread's queue_lock.
         *
         * If we do not grab the queue_lock here,
         * a potential wrong case may happen:
         *
         * CPU-0: handle_timer_irq -> T is timeout and sched_equeued to CPU-1
         *      CPU-1: T starts to run and invokes nanosleep again.
         *             Thus, enqueue_sleeper is invoked (cb is set)
         * CPU-1: handle_timer_irq -> continues to run and sets cb to NULL,
         *                            i.e., override the setted cb.
         */
        lock(&current_thread->sleep_state.queue_lock);

        enqueue_sleeper(current_thread, &ts_k, sleep_timer_cb);

        unlock(&current_thread->sleep_state.queue_lock);

        thread_set_ts_blocking(current_thread);

        /* Set the return value for nanosleep. */
        arch_set_thread_return(current_thread, 0);

        sched();
        eret_to_thread(switch_context());
        BUG("should not reach here\n");

        return 0;
}
