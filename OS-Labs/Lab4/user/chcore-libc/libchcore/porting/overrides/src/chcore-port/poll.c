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

#include <atomic.h>
#include <bits/alltypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall_arch.h>
#include <time.h>
#include <sys/time.h>
#include <raw_syscall.h>
#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore-internal/lwip_defs.h>

#include "fd.h"
#include "poll.h"

#include "socket.h" /* socket server poll function */
#include "timerfd.h" /* timerfd async poll function */

// #define chcore_poll_debug

#ifdef chcore_poll_debug
#define poll_debug(fmt, ...) \
        printf("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define poll_debug(fmt, ...)
#endif

#define warn_once(fmt, ...)               \
        do {                              \
                static int __warned = 0;  \
                if (__warned)             \
                        break;            \
                __warned = 1;             \
                warn(fmt, ##__VA_ARGS__); \
        } while (0)


/* epoll operation */
int chcore_epoll_create1(int flags)
{
        int epfd = 0, ret = 0;
        struct fd_desc *epoll_fd_desc;
        struct eventpoll *ep = NULL;

        epfd = alloc_fd();
        if (epfd < 0) {
                ret = epfd;
                goto fail;
        }

        if ((ep = malloc(sizeof(*ep))) == NULL) {
                ret = -ENOMEM;
                goto fail;
        }
        ep->epi_lock = 0;
        init_list_head(&ep->epi_list);
        ep->wait_count = 0;

        epoll_fd_desc = fd_dic[epfd];
        epoll_fd_desc->fd = epfd;
        epoll_fd_desc->type = FD_TYPE_EPOLL;
        epoll_fd_desc->private_data = ep;
        epoll_fd_desc->flags = flags;
        epoll_fd_desc->fd_op = &epoll_ops;

        return epfd;

fail:
        if (epfd >= 0)
                free_fd(epfd);
        if (ep > 0)
                free(ep);
        return ret;
}

int chcore_epoll_ctl(int epfd, int op, int fd, struct epoll_event *events)
{
        struct epitem *epi = NULL, *tmp = NULL;
        struct eventpoll *ep;
        int ret = 0;
        int fd_exist = 0;

        /* EINVAL events is NULL */
        if (events == NULL && op != EPOLL_CTL_DEL)
                return -EINVAL;

        /* EBADF  epfd or fd is not a valid file descriptor. */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL)
                return -EBADF;

        /* EBADF  epfd or fd is not a valid file descriptor. */
        if (epfd < 0 || epfd >= MAX_FD || fd_dic[epfd] == NULL
            || fd_dic[epfd]->type != FD_TYPE_EPOLL
            || (ep = fd_dic[epfd]->private_data) == NULL)
                return -EBADF;

        /* EINVAL epfd is not an epoll file descriptor, or fd is the same as
         * epfd */
        if (fd == epfd || fd_dic[epfd]->type != FD_TYPE_EPOLL)
                return -EINVAL;

        /* ELOOP  fd refers to an epoll instance and this EPOLL_CTL_ADD
         * operation would result in a circular loop of epoll instances
         * monitoring one another. (CHCORE NOT SUPPORT HERE) */
        if (fd_dic[fd]->type == FD_TYPE_EPOLL)
                return -ELOOP;

        /* EPERM  The target file fd does not support epoll.  This error can
         * occur if fd refers to, for example, a regular file or a directory. */
        if (fd_dic[fd]->fd_op == NULL)
                return -EPERM;

        chcore_spin_lock(&ep->epi_lock);
        switch (op) {
        case EPOLL_CTL_ADD:
                /* Check if already exist */
                for_each_in_list_safe (epi, tmp, epi_node, &ep->epi_list) {
                        if (epi->fd == fd) {
                                /* EEXIST op was EPOLL_CTL_ADD, and the supplied
                                 * file descriptor fd is already registered with
                                 * this epoll instance. */
                                ret = -EEXIST;
                                goto out;
                        }
                }
                if ((epi = malloc(sizeof(*epi))) == NULL) {
                        /* ENOMEM There was insufficient memory to handle the
                         * requested op control operation. */
                        ret = -ENOMEM;
                        goto out;
                }
                epi->fd = fd;
                epi->event = *events;
                list_append(&epi->epi_node, &ep->epi_list);
                ep->wait_count++;
                ret = 0;
                break;
        case EPOLL_CTL_MOD:
                /* Check if exist */
                fd_exist = 0;
                for_each_in_list_safe (epi, tmp, epi_node, &ep->epi_list) {
                        if (epi->fd == fd) {
                                epi->event = *events; /* Update the event */
                                fd_exist = 1; /* set the flag */
                                break;
                        }
                }
                /* ENOENT op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not
                 * registered with this epoll instance. */
                if (fd_exist == 0)
                        ret = -ENOENT;
                break;
        case EPOLL_CTL_DEL:
                /* Check if exist */
                fd_exist = 0;
                for_each_in_list_safe (epi, tmp, epi_node, &ep->epi_list) {
                        if (epi->fd == fd) {
                                list_del(&epi->epi_node); /* delete the node */
                                fd_exist = 1; /* set the flag */
                                break;
                        }
                }
                /* ENOENT op was EPOLL_CTL_MOD or EPOLL_CTL_DEL, and fd is not
                 * registered with this epoll instance. */
                if (fd_exist == 0)
                        ret = -ENOENT;
                break;
        default:
                /* The requested operation op is not supported by this interface
                 */
                ret = -ENOSYS;
        }

out:
        chcore_spin_unlock(&ep->epi_lock);
        return ret;
}

/*
 * XXX: Convert epoll to poll, which is not efficient as all fds
 * are traserved.
 */
int chcore_epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
                       int timeout, const sigset_t *sigmask)
{
        struct eventpoll *ep;
        struct epitem *epi = NULL, *tmp = NULL;
        struct pollfd *fds;
        epoll_data_t *epdata;
        int i = 0, ret = 0, ev_idx = 0, nfds = 0;
        sigset_t origmask;

        /* EBADF  epfd or fd is not a valid file descriptor. */
        if (epfd < 0 || epfd >= MAX_FD || fd_dic[epfd] == NULL
            || fd_dic[epfd]->type != FD_TYPE_EPOLL
            || (ep = fd_dic[epfd]->private_data) == NULL)
                return -EBADF;

        /* EINVAL epfd is not an epoll file descriptor, or maxevents is less
         * than or equal to zero. */
        if (fd_dic[epfd]->type != FD_TYPE_EPOLL || maxevents <= 0)
                return -EINVAL;

        /* EFAULT The memory area pointed to by events is not accessible with
         * write permissions. */
        if (events == NULL)
                return -EFAULT;

        if (sigmask)
                pthread_sigmask(SIG_SETMASK, sigmask, &origmask);

        chcore_spin_lock(&ep->epi_lock);
        if ((fds = (struct pollfd *)malloc(ep->wait_count * sizeof(*fds)))
            == NULL) {
                chcore_spin_unlock(&ep->epi_lock);
                return -ENOMEM;
        }
        if ((epdata = (epoll_data_t *)malloc(ep->wait_count
                                             * sizeof(epoll_data_t)))
            == NULL) {
                free(fds);
                chcore_spin_unlock(&ep->epi_lock);
                return -ENOMEM;
        }

        /* Convert epoll epi_list to fds */
        /* XXX: can be optimized by using the same format to store epoll_list */
        nfds = 0; /* index of fds */
        for_each_in_list_safe (epi, tmp, epi_node, &ep->epi_list) {
                fds[nfds].fd = epi->fd;
                /* EPOLLEXCLUSIVE, EPOLLWAKEUP, EPOLLONESHOT and EPOLLET are
                 * special */
                if (epi->event.events & EPOLLEXCLUSIVE
                    || epi->event.events & EPOLLWAKEUP
                    || epi->event.events & EPOLLONESHOT
                    || epi->event.events & EPOLLET)
                        warn_once(
                                "EPOLLEXCLUSIVE, EPOLLWAKEUP, EPOLLONESHOT and "
                                "EPOLLET not supported!");
                fds[nfds].events = epi->event.events & ~EPOLLEXCLUSIVE
                                   & ~EPOLLWAKEUP & ~EPOLLONESHOT & ~EPOLLET;
                /* Copy epdata */
                epdata[nfds] = epi->event.data;
                nfds++;
        }
        chcore_spin_unlock(&ep->epi_lock);

        ret = chcore_poll(fds, nfds, timeout);
        if (ret > 0) {
                ev_idx = 0; /* index of events */
                for (i = 0; i < nfds && ev_idx < maxevents; i++) {
                        if ((fds[i].revents & fds[i].events)) {
                                events[ev_idx].events = fds[i].revents;
                                events[ev_idx].data = epdata[i];
                                ev_idx++;
                        }
                }
        }
        poll_debug("epoll events:%d,%d\n", ret, ev_idx);

        free(fds);
        free(epdata);
        if (sigmask)
                pthread_sigmask(SIG_SETMASK, &origmask, NULL);
        return ret;
}

static ssize_t chcore_epoll_read(int fd, void *buf, size_t size)
{
        WARN("EPOLL CANNOT READ!\n");
        return -EINVAL;
}

static ssize_t chcore_epoll_write(int fd, void *buf, size_t size)
{
        WARN("EPOLL CANNOT WRITE!\n");
        return -EINVAL;
}

static int chcore_epoll_close(int fd)
{
        /* TODO Recycle threads */
        if (fd < 0 || fd >= MAX_FD || fd_dic[fd] == NULL
            || fd_dic[fd]->type != FD_TYPE_EPOLL
            || fd_dic[fd]->private_data == NULL)
                return -EBADF;

        free(fd_dic[fd]->private_data);
        free_fd(fd);
        return 0;
}

struct fd_ops epoll_ops = {
        .read = chcore_epoll_read,
        .write = chcore_epoll_write,
        .close = chcore_epoll_close,
        .poll = NULL,
        .ioctl = NULL,
        .fcntl = NULL,
};

int chcore_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
        int i, mask;
        struct pollarg arg;
        int count = 0;
        struct timeval start_time, current_time;
        long start_ms, current_ms; /* ms */

        /*
         * EFAULT fds points outside the process's accessible address space.
         * The array given as argument was not contained in the calling
         * program's address space.
         */
        if (fds == 0)
                return -EFAULT;

        for (i = 0; i < nfds; i++) {
                if (fds[i].fd < 0) {
                        /* Ignore negative fd, as man poll(2) described */
                        continue;
                } else if (fds[i].fd >= MAX_FD) {
                        printf("fd number (%d) not supported\n", fds[i].fd);
                        return -EINVAL;
                }
        }

        if (nfds == 0)
                return 0;

        poll_debug("Poll nfds:%d fds %p timeout:%d\n",
                   nfds, fds, timeout);

        /* Clear revents */
        for (i = 0; i < nfds; i++) {
                fds[i].revents = 0;
        }

        if (timeout > 0) {
                gettimeofday(&start_time, NULL);
                start_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;
        }

        while (true) {
                for (i = 0; i < nfds; i++) {
                        /*
                         * The field fd contains a file descriptor for an open
                         * file.  If this field is negative, then the
                         * corresponding events field is ignored and the revents
                         * field returns zero.  (This provides an easy way of
                         * ignoring a file descriptor for a single poll()
                         * call: simply negate the fd field.  Note, however,
                         * that this technique can't be used to ignore file
                         * descriptor 0.)
                         */
                        if (fds[i].fd < 0) {
                                /* False fd, just ignore them */
                                fds[i].revents = 0;
                                continue;
                        }

                        /*
                         * Invalid request: fd not open (only returned in
                         * revents; ignored in events). Or target fd does not
                         * support poll function.
                         *
                         * FIXME: The last condition is to unsupport those fds
                         * without lwip server cap.
                         */

                        if (fds[i].fd >= MAX_FD || fd_dic[fds[i].fd] == 0
                            || fd_dic[fds[i].fd]->fd_op == 0
                            || fd_dic[fds[i].fd]->fd_op->poll == 0) {
                                fds[i].revents = POLLNVAL;
                                continue;
                        }

                        arg.events = fds[i].events;

                        /* Call fd poll function */
                        mask = fd_dic[fds[i].fd]->fd_op->poll(fds[i].fd, &arg);
                        if (mask > 0) {
                                /* Already achieve the requirement */
                                fds[i].revents = mask;
                                count++;
                        }
                        poll_debug("poll fd %d mask 0x%x\n", fds[i].fd, mask);
                }

                if (count || timeout == 0)
                        break;

                usys_yield();

                if (timeout > 0) {
                        gettimeofday(&current_time, NULL);
                        current_ms = current_time.tv_sec * 1000
                                + current_time.tv_usec / 1000;
                        if ((current_ms - start_ms) > timeout)
                                break;
                }
        }

        poll_debug("poll return count %d\n", count);
        /* TODO: Free notifc (not supported yet) */
        return count;
}

int chcore_ppoll(struct pollfd *fds, nfds_t n, const struct timespec *tmo_p,
                 const sigset_t *sigmask)
{
        int timeout;
        int ret;
        sigset_t origmask;

        /* EINVAL (ppoll()) The timeout value expressed in *ip is invalid
         * (negative).*/
        if (tmo_p != NULL && (tmo_p->tv_sec < 0 || tmo_p->tv_nsec < 0))
                return -EINVAL;

        if (sigmask)
                pthread_sigmask(SIG_SETMASK, sigmask, &origmask);
        timeout = (tmo_p == NULL) ?
                          -1 :
                          (tmo_p->tv_sec * 1000 + tmo_p->tv_nsec / 1000000);
        ret = chcore_poll(fds, n, timeout);
        if (sigmask)
                pthread_sigmask(SIG_SETMASK, &origmask, NULL);
        return ret;
}
