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

#ifndef CHCORE_PORT_POLL_H
#define CHCORE_PORT_POLL_H

#include <debug_lock.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/epoll.h>

struct epitem {
        int fd;
        struct epoll_event event;
        /* epitem list in the same eventpoll */
        struct list_head epi_node;
};

struct eventpoll {
        /* All epitems */
        int volatile epi_lock;
        struct list_head epi_list;
        uint32_t wait_count;
};

/* Use by poll wait */
struct pollarg {
        /* Event mask */
        short int events;
};

int chcore_epoll_create1(int flags);
int chcore_epoll_ctl(int epfd, int op, int fd, struct epoll_event *events);
int chcore_epoll_pwait(int epfd, struct epoll_event *events, int maxevents,
                       int timeout, const sigset_t *sigmask);

int chcore_poll(struct pollfd fds[], nfds_t nfds, int timeout);
int chcore_ppoll(struct pollfd *fds, nfds_t n, const struct timespec *tmo_p,
                 const sigset_t *sigmask);

#endif /* CHCORE_PORT_POLL_H */
