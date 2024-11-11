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

#include <ipc/connection.h>
#include <ipc/notification.h>
#include <common/list.h>
#include <common/errno.h>
#include <object/thread.h>
#include <object/irq.h>
#include <object/object.h>
#include <sched/sched.h>
#include <sched/context.h>
#include <irq/irq.h>
#include <mm/uaccess.h>

int init_notific(struct notification *notifc)
{
        notifc->not_delivered_notifc_count = 0;
        notifc->waiting_threads_count = 0;
        init_list_head(&notifc->waiting_threads);
        lock_init(&notifc->notifc_lock);
        notifc->state = OBJECT_STATE_VALID;
        return 0;
}

void deinit_notific(struct notification *notifc)
{
        /* No deinitialization is required for now. */
}

void notification_deinit(void *ptr)
{
        deinit_notific((struct notification *)ptr);
}

/*
 * A waiting thread can be awoken by timeout and signal, which leads to racing.
 * We guarantee that a thread is not awoken for twice by 1. removing a thread
 * from notification waiting_threads when timeout and 2. removing a thread from
 * sleep_list when get signaled.
 * When signaled:
 *	lock(notification)
 *	remove from waiting_threads
 *      thread state = TS_READY
 *	unlock(notification)
 *
 *	if (sleep_state.cb != NULL) {
 *		lock(sleep_list)
 *		if (sleep_state.cb != NULL)
 *			remove from sleep_list
 *		unlock(sleep_list)
 *	}
 *
 * When timeout:
 *	lock(sleep_list)
 *	remove from sleep_list
 *	lock(notification)
 *	if (thread state == TS_BLOCKING)
 *		remove from waiting_threads
 *	unlock(notification)
 *	sleep_state.cb = NULL
 *	unlock(sleep_list)
 */

static void notific_timer_cb(struct thread *thread)
{
        struct notification *notifc;

        notifc = thread->sleep_state.pending_notific;
        thread->sleep_state.pending_notific = NULL;

        lock(&notifc->notifc_lock);

        /* For recycling: the state is set in stop_notification */
        if (notifc->state == OBJECT_STATE_INVALID) {
                thread_set_exited(thread);
                unlock(&notifc->notifc_lock);
                return;
        }

        if (!thread_is_ts_blocking(thread)) {
                unlock(&notifc->notifc_lock);
                return;
        }

        list_del(&thread->notification_queue_node);
        BUG_ON(notifc->waiting_threads_count <= 0);
        notifc->waiting_threads_count--;

        arch_set_thread_return(thread, -ETIMEDOUT);
        BUG_ON(sched_enqueue(thread));

        unlock(&notifc->notifc_lock);
}

/* Unlock futex lock iff eret_to_thread. */
int wait_notific_internal(struct notification *notifc, bool is_block,
                          struct timespec *timeout, bool need_unlock,
                          bool need_obj_put)
{
        int ret = 0;
        struct thread *thread;
        struct lock *futex_lock = NULL;

        lock(&notifc->notifc_lock);

        /* For recycling: the state is set in stop_notification */
        if (notifc->state == OBJECT_STATE_INVALID) {
                unlock(&notifc->notifc_lock);
                return -ECAPBILITY;
        }

        if (notifc->not_delivered_notifc_count > 0) {
                notifc->not_delivered_notifc_count--;
                ret = 0;
        } else {
                if (is_block) {
                        thread = current_thread;
                        /*
                         * queue_lock: grab the lock and then insert/remove
                         * a thread into one list.
                         */

                        lock(&thread->sleep_state.queue_lock);

                        /* Add this thread to waiting list */
                        list_append(&thread->notification_queue_node,
                                    &notifc->waiting_threads);
                        thread_set_ts_blocking(thread);
                        notifc->waiting_threads_count++;
                        arch_set_thread_return(thread, 0);

                        if (timeout) {
                                thread->sleep_state.pending_notific = notifc;
                                enqueue_sleeper(
                                        thread, timeout, notific_timer_cb);
                        }

                        if (need_unlock) {
                                futex_lock = &current_cap_group->futex_lock;
                        }

                        /*
                         * Since current_thread is TS_BLOCKING,
                         * sched() will not put current_thread into the
                         * ready_queue.
                         *
                         * sched() must executed before unlock.
                         * Otherwise, current_thread maybe be notified and then
                         * its state will be set to TS_RUNNING. If so, sched()
                         * will put it into the ready_queue and it maybe
                         * directly switch to.
                         */
                        sched();

                        unlock(&thread->sleep_state.queue_lock);

                        unlock(&notifc->notifc_lock);

                        /* See the below impl of sys_notify */
                        if (need_obj_put) {
                                obj_put(notifc);
                        }

                        if (need_unlock) {
                                unlock(futex_lock);
                        }

                        eret_to_thread(switch_context());
                        /* The control flow will never reach here */
                } else {
                        ret = -EAGAIN;
                }
        }
        unlock(&notifc->notifc_lock);
        return ret;
}

/* Return 0 if wait successfully, -EAGAIN otherwise */
int wait_notific(struct notification *notifc, bool is_block,
                 struct timespec *timeout)
{
        return wait_notific_internal(notifc, is_block, timeout, false, true);
}

void wait_irq_notific(struct irq_notification *irq_notifc)
{
        struct notification *notifc;

        notifc = &(irq_notifc->notifc);
        lock(&notifc->notifc_lock);

        /* Add this thread to waiting list */
        list_append(&current_thread->notification_queue_node,
                    &notifc->waiting_threads);
        thread_set_ts_blocking(current_thread);
        notifc->waiting_threads_count++;
        arch_set_thread_return(current_thread, 0);

        irq_notifc->user_handler_ready = 1;

        sched();

        unlock(&notifc->notifc_lock);

        eret_to_thread(switch_context());
        /* The control flow will never reach here */
}

void signal_irq_notific(struct irq_notification *irq_notifc)
{
        struct notification *notifc;
        struct thread *target;

        notifc = &(irq_notifc->notifc);

        lock(&notifc->notifc_lock);

        irq_notifc->user_handler_ready = 0;

        /*
         * Some threads have been blocked and waiting for notifc.
         * Wake up one waiting thread
         */
        target = list_entry(notifc->waiting_threads.next,
                            struct thread,
                            notification_queue_node);
        list_del(&target->notification_queue_node);
        notifc->waiting_threads_count--;

        BUG_ON(target->thread_ctx->sc == NULL);

        unlock(&notifc->notifc_lock);
        obj_put(irq_notifc);

        BUG_ON(sched_enqueue(target));

        sched_periodic();
        eret_to_thread(switch_context());
}

void try_remove_timeout(struct thread *target)
{
        if (target == NULL)
                return;
        if (target->sleep_state.cb == NULL)
                return;

        try_dequeue_sleeper(target);

        target->sleep_state.pending_notific = NULL;
}

int signal_notific(struct notification *notifc)
{
        struct thread *target = NULL;

        lock(&notifc->notifc_lock);

        /* For recycling: the state is set in stop_notification */
        if (notifc->state == OBJECT_STATE_INVALID) {
                unlock(&notifc->notifc_lock);
                return -ECAPBILITY;
        }

        if (notifc->not_delivered_notifc_count > 0
            || notifc->waiting_threads_count == 0) {
                notifc->not_delivered_notifc_count++;
        } else {
                /*
                 * Some threads have been blocked and waiting for notifc.
                 * Wake up one waiting thread
                 */
                target = list_entry(notifc->waiting_threads.next,
                                    struct thread,
                                    notification_queue_node);

                BUG_ON(target == NULL);

                /*
                 * signal_notific may return -EAGAIN because of unable to lock.
                 * The user-level library will transparently notify again.
                 *
                 * This is for preventing dead lock because handler_timer_irq
                 * may already grab the queue_lock of a thread or the sleep_list
                 * lock.
                 */
                if (try_lock(&target->sleep_state.queue_lock) != 0) {
                        /* Lock failed: must be timeout now */
                        unlock(&notifc->notifc_lock);
                        return -EAGAIN;
                }

                /* cb != NULL indicates the thread is also in the sleep list */
                if (target->sleep_state.cb != NULL) {
                        if (try_dequeue_sleeper(target) == false) {
                                /* Failed to remove target in sleep list */
                                unlock(&target->sleep_state.queue_lock);
                                unlock(&notifc->notifc_lock);
                                return -EAGAIN;
                        }
                }

                /* Delete the thread from the waiting list of the notification
                 */
                list_del(&target->notification_queue_node);
                notifc->waiting_threads_count--;

                BUG_ON(sched_enqueue(target));

                unlock(&target->sleep_state.queue_lock);
        }

        unlock(&notifc->notifc_lock);

        return 0;
}

/* For FUTEX_REQUEUE only */
int requeue_notific(struct notification *src_notifc,
                    struct notification *dst_notifc)
{
        struct thread *target = NULL;
        struct lock *big_lock = &dst_notifc->notifc_lock,
                    *small_lock = &src_notifc->notifc_lock;
        int ret = 0;

        if (src_notifc == dst_notifc)
                return 0;

        if (src_notifc > dst_notifc) {
                big_lock = &src_notifc->notifc_lock;
                small_lock = &dst_notifc->notifc_lock;
        }
        lock(small_lock);
        lock(big_lock);

        /* For recycling: the state is set in stop_notification */
        if (src_notifc->state == OBJECT_STATE_INVALID
            || dst_notifc->state == OBJECT_STATE_INVALID) {
                ret = -ECAPBILITY;
                goto out_unlock;
        }
        if (src_notifc->waiting_threads_count == 0) {
                ret = 0;
                goto out_unlock;
        }

        target = list_entry(src_notifc->waiting_threads.next,
                            struct thread,
                            notification_queue_node);

        BUG_ON(target == NULL);

        if (try_lock(&target->sleep_state.queue_lock) != 0) {
                /* Lock failed: must be timeout now */
                ret = -EAGAIN;
                goto out_unlock;
        }

        /* Delete the thread from the waiting list of the src notification*/
        list_del(&target->notification_queue_node);
        src_notifc->waiting_threads_count--;

        /* Add this thread to dst waiting list */
        if (dst_notifc->not_delivered_notifc_count > 0) {
                BUG("Futex code should guarantee ZERO not_delivered_notifc_count.");
        } else {
                target->sleep_state.pending_notific = dst_notifc;
                list_append(&target->notification_queue_node,
                            &dst_notifc->waiting_threads);
                dst_notifc->waiting_threads_count++;
        }

        unlock(&target->sleep_state.queue_lock);

out_unlock:
        unlock(big_lock);
        unlock(small_lock);

        return ret;
}

CAP_ALLOC_IMPL(create_notific,
               TYPE_NOTIFICATION,
               sizeof(struct notification),
               CAP_ALLOC_OBJ_FUNC(init_notific),
               CAP_ALLOC_OBJ_FUNC(notification_deinit));

cap_t sys_create_notifc(void)
{
        return CAP_ALLOC_CALL(create_notific, current_cap_group, CAP_RIGHT_NO_RIGHTS);
}

int sys_wait(cap_t notifc_cap, bool is_block, struct timespec *timeout)
{
        struct notification *notifc;
        struct timespec timeout_k;
        int ret;

        notifc = obj_get(
                current_thread->cap_group, notifc_cap, TYPE_NOTIFICATION);
        if (!notifc) {
                ret = -ECAPBILITY;
                goto out;
        }

        if (timeout) {
                ret = copy_from_user(&timeout_k, timeout, sizeof(timeout_k));
                if (ret != 0)
                        goto out_obj_put;
        }

        ret = wait_notific(notifc, is_block, timeout ? &timeout_k : NULL);
out_obj_put:
        obj_put(notifc);
out:
        return ret;
}

int sys_notify(cap_t notifc_cap)
{
        struct notification *notifc;
        int ret;
        notifc = obj_get(
                current_thread->cap_group, notifc_cap, TYPE_NOTIFICATION);
        if (!notifc) {
                ret = -ECAPBILITY;
                goto out;
        }
        ret = signal_notific(notifc);
        obj_put(notifc);
out:
        return ret;
}
