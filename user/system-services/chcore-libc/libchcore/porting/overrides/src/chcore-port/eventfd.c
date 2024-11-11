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
#include <debug_lock.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall_arch.h>
#include <time.h>
#include <pthread.h>
#include <raw_syscall.h>

#include <chcore/bug.h>
#include <chcore/container/list.h>
#include <chcore/defs.h>

#include "eventfd.h"
#include "fd.h"
#include "poll.h"

struct eventfd {
        uint64_t efd_val; /* 64-bit efd_val */
        int volatile efd_lock;
        cap_t writer_notifc_cap;
        uint64_t writer_waiter_cnt;
        cap_t reader_notifc_cap;
        uint64_t reader_waiter_cnt;
};

int chcore_eventfd(unsigned int count, int flags)
{
        int fd = 0;
        int ret = 0;
        struct eventfd *efdp = 0;
        cap_t notifc_cap = 0;

        if ((fd = alloc_fd()) < 0) {
                ret = fd;
                goto fail;
        }
        fd_dic[fd]->flags = flags;
        fd_dic[fd]->type = FD_TYPE_EVENT;
        fd_dic[fd]->fd_op = &event_op;

        if ((efdp = malloc(sizeof(struct eventfd))) <= 0) {
                ret = -ENOMEM;
                goto fail;
        }
        /* use efd_val to store the initval */
        efdp->efd_val = count;
        /* init per eventfd lock */
        efdp->efd_lock = 0;
        /* alloc notification for eventfd */
        if ((notifc_cap = chcore_syscall0(CHCORE_SYS_create_notifc)) < 0) {
                ret = notifc_cap;
                goto fail;
        }
        efdp->reader_notifc_cap = notifc_cap;
        efdp->reader_waiter_cnt = 0;

        if ((notifc_cap = chcore_syscall0(CHCORE_SYS_create_notifc)) < 0) {
                ret = notifc_cap;
                goto fail;
        }
        efdp->writer_notifc_cap = notifc_cap;
        efdp->writer_waiter_cnt = 0;

        a_barrier();
        fd_dic[fd]->private_data = efdp;

        return fd;
fail:
        if (fd >= 0)
                free_fd(fd);
        if (efdp > 0)
                free(efdp);
        return ret;
}

static ssize_t chcore_eventfd_read(int fd, void *buf, size_t count)
{
        /* Already checked fd before call this function */
        BUG_ON(!fd_dic[fd]->private_data);

        /* A read fails with the error EINVAL if the size
           of the supplied buffer is less than 8 bytes. */
        if (count < sizeof(uint64_t))
                return -EINVAL;

        struct eventfd *efdp = fd_dic[fd]->private_data;
        uint64_t val = 0, i = 0;
        int ret = 0;

        while (1) {
                chcore_spin_lock(&efdp->efd_lock);

                /* Fast Path */
                if ((val = efdp->efd_val) != 0) {
                        if (fd_dic[fd]->flags & EFD_SEMAPHORE) {
                                /* EFD as semaphore */
                                efdp->efd_val--;
                                *(uint64_t *)buf = 1;
                                /* At most 1 writer can be waked */
                                if (efdp->writer_waiter_cnt) {
                                        // ret = chcore_syscall1(CHCORE_SYS_notify,
                                        //		      efdp->writer_notifc_cap);
                                        ret = usys_notify(efdp->writer_notifc_cap);
                                        efdp->writer_waiter_cnt -= 1;
                                }
                        } else {
                                efdp->efd_val = 0;
                                *(uint64_t *)buf = val;
                                /* At most `val` writer can be waked */
                                if (efdp->writer_waiter_cnt) {
                                        for (i = 0;
                                        i < efdp->writer_waiter_cnt && i < val;
                                        i++)
                                                // ret =
                                                // chcore_syscall1(CHCORE_SYS_notify,
                                                //   efdp->writer_notifc_cap);
                                                ret = usys_notify(
                                                        efdp->writer_notifc_cap);
                                        efdp->writer_waiter_cnt -= i;
                                }
                        }

                        ret = sizeof(uint64_t);
                        goto out;
                }

                /* Slow Path */
                if (fd_dic[fd]->flags & EFD_NONBLOCK) {
                        ret = -EAGAIN;
                        goto out;
                }

                /* going to wait notific */
                BUG_ON(efdp->reader_waiter_cnt == 0xffffffffffffffff);
                efdp->reader_waiter_cnt++;
                chcore_spin_unlock(&efdp->efd_lock);

                ret = chcore_syscall3(
                        CHCORE_SYS_wait, efdp->reader_notifc_cap, true, 0);
                BUG_ON(ret);
        }

out:
        chcore_spin_unlock(&efdp->efd_lock);
        return ret;
}

static ssize_t chcore_eventfd_write(int fd, void *buf, size_t count)
{
        /* Already checked fd before call this function */
        BUG_ON(!fd_dic[fd]->private_data);

        if (count < sizeof(uint64_t))
                return -EINVAL;
        if (*(uint64_t *)buf == 0xffffffffffffffff)
                return -EINVAL;

        struct eventfd *efdp = fd_dic[fd]->private_data;
        int ret = 0;
        uint64_t i = 0;

        while (1) {
                /* Should wait until all value has written */
                chcore_spin_lock(&efdp->efd_lock);
                /* Fast Path */
                if (*(uint64_t *)buf < 0xffffffffffffffff - efdp->efd_val) {
                        efdp->efd_val += *(uint64_t *)buf;
                        if (efdp->reader_waiter_cnt) {
                                /* wake up reader */
                                if (fd_dic[fd]->flags & EFD_SEMAPHORE) {
                                        /* sem mode */
                                        for (i = 0; i < efdp->reader_waiter_cnt
                                                    && i < efdp->efd_val;
                                             i++)
                                                // ret =
                                                // chcore_syscall1(CHCORE_SYS_notify,
                                                // efdp->reader_notifc_cap);
                                                ret = usys_notify(
                                                        efdp->reader_notifc_cap);
                                        efdp->reader_waiter_cnt -= i;
                                } else {
                                        // ret =
                                        // chcore_syscall1(CHCORE_SYS_notify,
                                        // efdp->reader_notifc_cap);
                                        ret = usys_notify(
                                                efdp->reader_notifc_cap);
                                        efdp->reader_waiter_cnt -= 1;
                                }
                        }
                        BUG_ON(ret != 0);
                        ret = sizeof(uint64_t);
                        goto out;
                }

                /* Slow Path */
                if (fd_dic[fd]->flags & EFD_NONBLOCK) {
                        ret = -EAGAIN;
                        goto out;
                }

                /* going to wait notific */
                BUG_ON(efdp->writer_waiter_cnt == 0xffffffffffffffff);
                efdp->writer_waiter_cnt++;
                chcore_spin_unlock(&efdp->efd_lock);

                ret = chcore_syscall3(
                        CHCORE_SYS_wait, efdp->writer_notifc_cap, true, 0);
                BUG_ON(ret);
        }

out:
        chcore_spin_unlock(&efdp->efd_lock);
        return ret;
}

static int chcore_eventfd_close(int fd)
{
        BUG_ON(!fd_dic[fd]->private_data);

        free(fd_dic[fd]->private_data);
        free_fd(fd);
        return 0;
}

static int chcore_eventfd_poll(int fd, struct pollarg *arg)
{
        /* Already checked fd before call this function */
        BUG_ON(!fd_dic[fd]->private_data);
        int mask = 0;
        struct eventfd *efdp = fd_dic[fd]->private_data;

        /* Only check no need to lock */
        if (arg->events & POLLIN || arg->events & POLLRDNORM) {
                // printf("CHECK POLLIN fd %d efdp->efd_val %d\n", fd,
                // efdp->efd_val);
                /* Check whether can read */
                mask |= efdp->efd_val ? POLLIN | POLLRDNORM : 0;
        }

        /* Only check no need to lock */
        if (arg->events & POLLOUT || arg->events & POLLWRNORM) {
                /* Check whether can write */
                mask |= efdp->efd_val < 0xfffffffffffffffe ?
                                POLLOUT | POLLWRNORM :
                                0;
        }

        return mask;
}

/* EVENTFD */
struct fd_ops event_op = {
        .read = chcore_eventfd_read,
        .write = chcore_eventfd_write,
        .close = chcore_eventfd_close,
        .poll = chcore_eventfd_poll,
        .ioctl = NULL,
        .fcntl = NULL,
};
