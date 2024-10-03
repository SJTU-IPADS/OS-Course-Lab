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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <syscall_arch.h>

#include "fd.h"

#include <chcore/bug.h>
#include <chcore/syscall.h>

struct timer_file {
        struct itimerspec it;
        struct timeval start_time;
        int volatile timer_lock;
};

#define NS_IN_S (1000000000ULL)
static uint64_t timespec_to_ns(struct timespec *time)
{
        return (uint64_t)(NS_IN_S * (uint64_t)time->tv_sec +
                          (uint64_t)time->tv_nsec);
}

static struct timespec ns_to_timespec(uint64_t ns)
{
        struct timespec time;
        time.tv_sec = (time_t)(ns / NS_IN_S);
        time.tv_nsec = (long)(ns % NS_IN_S);
        return time;
}

int chcore_timerfd_create(int clockid, int flags)
{
        int timerfd;
        struct fd_desc *timerfd_desc;
        struct timer_file *timer_file;
        int ret = 0;

        if (clockid != CLOCK_MONOTONIC) {
                WARN("timerfd_create: clockid not support. use CLOCK_MONOTONIC "
                     "instead\n");
                clockid = CLOCK_MONOTONIC;
        }

        if (flags & TFD_CLOEXEC) {
                WARN("timerfd_create: flag TFD_CLOEXEC is omitted\n");
                flags &= ~TFD_CLOEXEC;
        }

        if ((timer_file = malloc(sizeof(*timer_file))) == NULL)
                return -ENOMEM;

        timer_file->it = (struct itimerspec){0};
        timer_file->timer_lock = 0;

        timerfd = alloc_fd();
        if (timerfd < 0) { /* failed */
                ret = timerfd;
                goto fail_out;
        }
        timerfd_desc = fd_dic[timerfd];
        timerfd_desc->flags = flags;
        timerfd_desc->type = FD_TYPE_TIMER;
        timerfd_desc->private_data = timer_file;
        timerfd_desc->fd_op = &timer_op;

        return timerfd;
fail_out:
        if (timer_file > 0)
                free(timer_file);
        return ret;
}

int chcore_timerfd_settime(int fd, int flags, struct itimerspec *new_value,
                           struct itimerspec *old_value)
{
        struct timer_file *timer_file;
        struct timespec cur_time;
        uint64_t cur_ns, val_ns;

        /* EBADF  fd is not a valid file descriptor. */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL)
                return -EBADF;

        /* EINVAL fd is not a valid timerfd file descriptor. */
        if (fd_dic[fd]->type != FD_TYPE_TIMER
            || fd_dic[fd]->private_data == NULL)
                return -EINVAL;

        /* EFAULT new_value, old_value, or curr_value is not valid a pointer. */
        if (new_value == NULL)
                return -EFAULT;

        /* EINVAL new_value is not properly initialized (one of the tv_nsec
         * falls outside the range zero to 999,999,999). */
        if (new_value->it_value.tv_nsec < 0
            || new_value->it_value.tv_nsec > 999999999
            || new_value->it_interval.tv_nsec < 0
            || new_value->it_interval.tv_nsec > 999999999)
                return -EINVAL;

        if (flags & TFD_TIMER_CANCEL_ON_SET) {
                WARN("timerfd_settime: flag TFD_TIMER_CANCEL_ON_SET omitted\n");
                flags &= ~TFD_TIMER_CANCEL_ON_SET;
        }

        timer_file = fd_dic[fd]->private_data;
        chcore_spin_lock(&timer_file->timer_lock);
        /* Passing the old_value */
        if (old_value) {
                old_value->it_interval = timer_file->it.it_interval;
                old_value->it_value = timer_file->it.it_value;
        }
        /* Set the time to expire. */
        timer_file->it = *new_value;
        if (new_value->it_value.tv_sec != 0
            || new_value->it_value.tv_nsec != 0) {
                val_ns = timespec_to_ns(&new_value->it_value);
                clock_gettime(CLOCK_MONOTONIC, &cur_time);
                cur_ns = timespec_to_ns(&cur_time);
                if (flags & TFD_TIMER_ABSTIME)
                        timer_file->it.it_value = ns_to_timespec(
                                cur_ns > val_ns ? cur_ns : val_ns);
                else
                        timer_file->it.it_value =
                                ns_to_timespec(cur_ns + val_ns);
        }
        chcore_spin_unlock(&timer_file->timer_lock);
        return 0;
}

int chcore_timerfd_gettime(int fd, struct itimerspec *curr_value)
{
        uint64_t cur_ns, val_ns, interval_ns, remain_ns;
        struct timespec cur_time;
        struct timer_file *timer_file;

        /* EBADF  fd is not a valid file descriptor. */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL)
                return -EBADF;

        /* EINVAL fd is not a valid timerfd file descriptor. */
        if (fd_dic[fd]->type != FD_TYPE_TIMER
            || fd_dic[fd]->private_data == NULL)
                return -EINVAL;

        /* EFAULT new_value, old_value, or curr_value is not valid a pointer. */
        if (curr_value == NULL)
                return -EFAULT;

        timer_file = fd_dic[fd]->private_data;

        chcore_spin_lock(&timer_file->timer_lock);
        curr_value->it_interval = timer_file->it.it_interval;
        if (timer_file->it.it_value.tv_sec == 0
            && timer_file->it.it_value.tv_nsec == 0) {
                curr_value->it_value = timer_file->it.it_value;
        } else {
                clock_gettime(CLOCK_MONOTONIC, &cur_time);
                cur_ns = timespec_to_ns(&cur_time);
                val_ns = timespec_to_ns(&timer_file->it.it_value);
                interval_ns = timespec_to_ns(&timer_file->it.it_interval);

                /* The it_value field returns the amount of time until the timer
                 * will next expire.  If both fields of this structure are zero,
                 * then the timer is currently disarmed.  This field always
                 * contains a relative value, regardless of whether the
                 * TFD_TIMER_ABSTIME flag was specified
                 * when setting the timer. */
                if (cur_ns < val_ns)
                        remain_ns = val_ns - cur_ns;
                else
                        remain_ns =
                                interval_ns - (cur_ns - val_ns) % interval_ns;
                curr_value->it_value = ns_to_timespec(remain_ns);
        }
        chcore_spin_unlock(&timer_file->timer_lock);
        return 0;
}

static ssize_t chcore_timerfd_read(int fd, void *buf, size_t count)
{
        uint64_t cur_ns, val_ns, interval_ns, next_expire_ns, tick;
        struct timespec cur_time, sleep_time;
        struct timer_file *timer_file;

        /* EBADF  fd is not a valid file descriptor. */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL)
                return -EBADF;

        /* EINVAL fd is not a valid timerfd file descriptor. */
        if (fd_dic[fd]->type != FD_TYPE_TIMER
            || fd_dic[fd]->private_data == NULL)
                return -EINVAL;

        if (count < sizeof(long))
                return -EINVAL;

        if (buf == NULL)
                return -EINVAL;

        timer_file = fd_dic[fd]->private_data;
        chcore_spin_lock(&timer_file->timer_lock);
        val_ns = timespec_to_ns(&timer_file->it.it_value);
        interval_ns = timespec_to_ns(&timer_file->it.it_interval);
        /* Check whether the timer has been disarmed */
        if (interval_ns == 0 && val_ns == 0) {
                chcore_spin_unlock(&timer_file->timer_lock);
                *(uint64_t *)buf = 0;
                return sizeof(tick);
        }
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        cur_ns = timespec_to_ns(&cur_time);
        /* Non-blocking mode */
        if (cur_ns < val_ns && (fd_dic[fd]->flags & TFD_NONBLOCK)) {
                chcore_spin_unlock(&timer_file->timer_lock);
                return -EAGAIN;
        }
        if (cur_ns < val_ns) {
                sleep_time = ns_to_timespec(val_ns - cur_ns);
                nanosleep(&sleep_time, NULL);
                clock_gettime(CLOCK_MONOTONIC, &cur_time);
                cur_ns = timespec_to_ns(&cur_time);
                val_ns = timespec_to_ns(&timer_file->it.it_value);
        }
        /* If both fields of new_value.it_interval are zero, the timer expires
         * just once, at the time specified by new_value.it_value. */
        if (interval_ns > 0) {
                tick = (cur_ns - val_ns) / interval_ns + 1;
                next_expire_ns =
                        cur_ns + interval_ns - (cur_ns - val_ns) % interval_ns;
        } else {
                tick = 1;
                next_expire_ns = 0;
        }
        *(uint64_t *)buf = tick;
        /* update timer for the next read */
        timer_file->it.it_value = ns_to_timespec(next_expire_ns);
        chcore_spin_unlock(&timer_file->timer_lock);
        return sizeof(tick);
}

static int chcore_timerfd_close(int fd)
{
        BUG_ON(!fd_dic[fd]->private_data);

        /* maybe we should lock the file */
        free(fd_dic[fd]->private_data);
        free_fd(fd);
        return 0;
}

static ssize_t chcore_timerfd_write(int fd, void *buf, size_t count)
{
        return -EINVAL;
}

static int chcore_timerfd_poll(int fd, struct pollarg *arg)
{
        uint64_t cur_ns, val_ns;
        struct timespec cur_time;
        struct timer_file *timer_file;
        int mask = 0;

        /* Check fd */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL
            || fd_dic[fd]->type != FD_TYPE_TIMER
            || fd_dic[fd]->private_data == NULL)
                return -EINVAL;

        if (arg->events & POLLIN || arg->events & POLLRDNORM) {
                timer_file = fd_dic[fd]->private_data;
                /* no need to lock, only check the value of it_value */
                val_ns = timespec_to_ns(&timer_file->it.it_value);
                if (val_ns != 0) {
                        clock_gettime(CLOCK_MONOTONIC, &cur_time);
                        cur_ns = timespec_to_ns(&cur_time);
                        mask = cur_ns >= val_ns ? POLLIN | POLLRDNORM : 0;
                }
        }

        return mask;
}

struct timerfd_poll_arg {
        uint64_t wait_ns;
        cap_t notifc_cap;
};

static void *timerfd_poll_thread_routine(void *arg)
{
        struct timerfd_poll_arg *tpa = (struct timerfd_poll_arg *)arg;
        struct timespec wait_time;

        wait_time = ns_to_timespec(tpa->wait_ns);
        nanosleep(&wait_time, NULL);
        // chcore_syscall1(CHCORE_SYS_notify, tpa->notifc_cap);
        usys_notify(tpa->notifc_cap);
        free(arg);
        return NULL;
}

int chcore_timerfd_async_poll(struct pollfd fds[], nfds_t nfds, int timeout,
                              cap_t notifc_cap)
{
        int ret = 0, i = 0, count = 0, fd = 0;
        pthread_t tid;
        struct timer_file *timer_file;
        struct timespec cur_time;
        uint64_t wait_ns = UINT64_MAX;
        uint64_t val_ns = 0, cur_ns = 0;
        struct timerfd_poll_arg *tpa;

        /* if timeout == 0, we have check in fd_op->poll function directly
         * return */
        if (timeout == 0)
                return 0;
        /* Find timeout */
        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        cur_ns = timespec_to_ns(&cur_time);
        for (i = 0; i < nfds; i++) {
                fd = fds[i].fd;
                if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL
                    || fd_dic[fd]->type != FD_TYPE_TIMER
                    || fd_dic[fd]->private_data == NULL)
                        continue; /* not a valid timerfd */
                /* get timeout */
                timer_file = fd_dic[fd]->private_data;
                val_ns = timespec_to_ns(&timer_file->it.it_value);
                if (val_ns == 0) { /* timer is disarmed */
                        continue;
                }
                if (cur_ns > val_ns) { /* ready */
                        fds[i].revents =
                                fds[i].events & POLLIN
                                                || fds[i].events & POLLRDNORM ?
                                        POLLIN | POLLRDNORM :
                                        0;
                        count += 1;
                } else { /* not ready */
                        wait_ns = (val_ns - cur_ns) < wait_ns ?
                                          (val_ns - cur_ns) :
                                          wait_ns;
                }
        }

        /* directly return if ready */
        if (count > 0)
                return count;

        /* OPT: directly return when timeout is closer */
        if (timeout > 0 && wait_ns > (uint64_t)timeout * 1000000)
                return 0;

        tpa = malloc(sizeof(struct timerfd_poll_arg));
        if (tpa == NULL) {
                return -ENOMEM;
        }
        tpa->notifc_cap = notifc_cap;
        tpa->wait_ns = wait_ns;
        ret = pthread_create(
                &tid, NULL, timerfd_poll_thread_routine, (void *)tpa);
        return ret;
}

/* TIMERFD */
struct fd_ops timer_op = {
        .read = chcore_timerfd_read,
        .write = chcore_timerfd_write,
        .close = chcore_timerfd_close,
        .poll = chcore_timerfd_poll,
        .ioctl = NULL,
        .fcntl = NULL,
};
