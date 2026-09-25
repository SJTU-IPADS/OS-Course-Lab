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

#ifndef IRQ_TIMER_H
#define IRQ_TIMER_H

#include <common/types.h>
#include <common/list.h>
#include <common/lock.h>
#include <machine.h>

struct thread;

typedef void (*timer_cb)(struct thread *thread);

/* Every thread has a sleep_state struct */
struct sleep_state {
	/* Link sleeping threads on each core */
	struct list_head sleep_node;
	/* Time to wake up */
	u64 wakeup_tick;
	/* The cpu id where the thread is sleeping */
	u64 sleep_cpu;
	/*
	 * When the timeout is caused during notification wait (with timeout),
	 * The following field indicates which notification it is waiting for.
	 * Currently, we record this field for removing the timeout thread from
	 * the waiting queue of the corresponding notification.
	 */
	struct notification *pending_notific;
	/*
	 * Currently 2 type of callbacks: notification and sleep.
	 * If it is NULL, the thread is not waiting for timeout.
	 */
	timer_cb cb;

	/*
	 * Get this lock before inserting or removing the thread in or from
	 * a waiting list, i.e., sleep_list and wait_list of a notification.
	 */
	struct lock queue_lock;
};

#define NS_IN_S		(1000000000UL)
#define US_IN_S		(1000000UL)
#define NS_IN_US	(1000UL)
#define US_IN_MS        (1000UL)

extern struct list_head sleep_lists[PLAT_CPU_NUM];
extern u64 tick_per_us;
int enqueue_sleeper(struct thread *thread, const struct timespec *timeout,
		    timer_cb cb);
bool try_dequeue_sleeper(struct thread *thread);

void timer_init(void);
void plat_timer_init(void);
void plat_set_next_timer(u64 tick_delta);
void handle_timer_irq(void);
void plat_handle_timer_irq(u64 tick_delta);

u64 plat_get_mono_time(void);
u64 plat_get_current_tick(void);

/* Syscalls */
int sys_clock_gettime(clockid_t clock, struct timespec *ts);
int sys_clock_nanosleep(clockid_t clk, int flags, const struct timespec *req,
                        struct timespec *rem);

#endif /* IRQ_TIMER_H */