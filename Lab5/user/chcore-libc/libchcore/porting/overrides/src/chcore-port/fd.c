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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <chcore/bug.h>
#include <limits.h>

#include "fd.h"
#include "fs_client_defs.h"

/*
 * No lock is used to protect fd_dic and its member
 * since we only modify the fd_desc when init by far ...
 */
/* Global fd desc table */
struct fd_desc *fd_dic[MAX_FD] = {0};
struct htable fs_cap_struct_htable;
/* Default fd operations */
struct fd_ops default_ops;

/*
 * Helper function.
 */
static int iov_check(const struct iovec *iov, int iovcnt)
{
        int iov_i;
        size_t iov_len_sum;

        if (iovcnt <= 0 || iovcnt > IOV_MAX)
                return -EINVAL;

        iov_len_sum = 0;
        for (iov_i = 0; iov_i < iovcnt; iov_i++)
                iov_len_sum += (iov + iov_i)->iov_len;

        return 0;
}

int alloc_fd(void)
{
        return alloc_fd_since(0);
}

int alloc_fd_since(int min)
{
        int cur_fd, ret;
        struct fd_desc *new_desc;
        struct fd_record_extension *fd_ext;

        /* Malloc fd_desc structure */
        new_desc = malloc(sizeof(struct fd_desc));
        if (new_desc == NULL) {
                return -ENOMEM;
        }
        /* Init fd_desc structure */
        memset(new_desc, 0, sizeof(struct fd_desc));
        /* Set default operation */
        new_desc->fd_op = &default_ops;

        /* XXX: performance */
        cur_fd = MIN_FD > min ? MIN_FD : min;
        for (; cur_fd < MAX_FD; cur_fd++) {
                if (!fd_dic[cur_fd]
                    && a_cas_p((void *)&fd_dic[cur_fd], 0, new_desc) == 0)
                        break;
        }
        if (cur_fd == MAX_FD) {
                ret = -ENFILE;
                goto out_free_new_desc;
        }

        /* libfs */
        fd_ext = _new_fd_record_extension();

        if (fd_ext == NULL) {
                ret = -ENOMEM;
                goto out_reset_cur_fd;
        }

        fd_dic[cur_fd]->private_data = (void *)fd_ext;
        return cur_fd;

out_reset_cur_fd:
        fd_dic[cur_fd] = NULL;

out_free_new_desc:
        free(new_desc);

        return ret;
}

/* XXX Concurrent problem */
void free_fd(int fd)
{
        free(fd_dic[fd]);
        fd_dic[fd] = 0;
}

/* fd opeartions */
ssize_t chcore_read(int fd, void *buf, size_t count)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        return fd_dic[fd]->fd_op->read(fd, buf, count);
}

ssize_t chcore_write(int fd, void *buf, size_t count)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        return fd_dic[fd]->fd_op->write(fd, buf, count);
}

ssize_t chcore_pread(int fd, void *buf, size_t count, off_t offset)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        if (fd_dic[fd]->fd_op->pread == NULL)
                return -EBADF;
        return fd_dic[fd]->fd_op->pread(fd, buf, count, offset);
}

ssize_t chcore_pwrite(int fd, void *buf, size_t count, off_t offset)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        if (fd_dic[fd]->fd_op->pwrite == NULL)
                return -EBADF;
        return fd_dic[fd]->fd_op->pwrite(fd, buf, count, offset);
}

int chcore_close(int fd)
{
        if (fd < 0 || fd > 1023 || fd_dic[fd] == 0)
                return -EBADF;
        return fd_dic[fd]->fd_op->close(fd);
}

int chcore_ioctl(int fd, unsigned long request, void *arg)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        return fd_dic[fd]->fd_op->ioctl(fd, request, arg);
}

int chcore_fcntl(int fd, int cmd, int arg)
{
        if (fd < 0 || fd_dic[fd] == 0)
                return -EBADF;
        switch (cmd) {
        case F_DUPFD_CLOEXEC:
        case F_DUPFD:
        case F_SETFL:
        case F_GETFL:
                return fd_dic[fd]->fd_op->fcntl(fd, cmd, arg);
        case F_GETFD: // gets fd flag
                return fd_dic[fd]->flags;
        case F_SETFD: // sets fd flag
                // Note: Posix only defines FD_CLOEXEC as file
                // descriptor flags. However it explicitly allows
                // implementation stores other status flags.
                fd_dic[fd]->flags = (arg & FD_CLOEXEC)
                                    | (fd_dic[fd]->flags & ~FD_CLOEXEC);
                return 0;
        case F_GETOWN:
                warn("fcntl (F_GETOWN, 0x%x, 0x%x) not implemeted, do "
                     "nothing.\n",
                     cmd,
                     arg);
                break;
        case F_SETOWN:
                warn("fcntl (F_SETOWN, 0x%x, 0x%x) not implemeted, do "
                     "nothing.\n",
                     cmd,
                     arg);
                break;
        case F_GETLK:
                warn("fcntl (F_GETLK, 0x%x, 0x%x) not implemeted, do "
                     "nothing.\n",
                     cmd,
                     arg);
                break;
        case F_SETLK:
                warn("fcntl (F_SETLK, 0x%x, 0x%x) not implemeted, do "
                     "nothing.\n",
                     cmd,
                     arg);
                break;
        case F_SETLKW:
                warn("fcntl (F_SETLKW, 0x%x, 0x%x) not implemeted, do "
                     "nothing.\n",
                     cmd,
                     arg);
                break;
        default:
                return -EINVAL;
        }
        return -1;
}

ssize_t chcore_readv(int fd, const struct iovec *iov, int iovcnt)
{
        int iov_i;
        ssize_t byte_read, ret;

        if ((ret = iov_check(iov, iovcnt)) != 0)
                return ret;

        byte_read = 0;
        for (iov_i = 0; iov_i < iovcnt; iov_i++) {
                ret = chcore_read(fd,
                                  (void *)((iov + iov_i)->iov_base),
                                  (size_t)(iov + iov_i)->iov_len);
                if (ret < 0) {
                        return ret;
                }

                byte_read += ret;
                if (ret != (iov + iov_i)->iov_len) {
                        return byte_read;
                }
        }

        return byte_read;
}

ssize_t chcore_writev(int fd, const struct iovec *iov, int iovcnt)
{
        int iov_i;
        ssize_t byte_written, ret;

        if ((ret = iov_check(iov, iovcnt)) != 0)
                return ret;

        byte_written = 0;
        for (iov_i = 0; iov_i < iovcnt; iov_i++) {
                ret = chcore_write(fd,
                                   (void *)((iov + iov_i)->iov_base),
                                   (size_t)(iov + iov_i)->iov_len);
                if (ret < 0) {
                        return ret;
                }

                byte_written += ret;
                if (ret != (iov + iov_i)->iov_len) {
                        return byte_written;
                }
        }
        return byte_written;
}

int dup_fd_content(int fd, int arg)
{
        int type, new_fd;
        if (arg == -1) {
                new_fd = alloc_fd();
        } else {
                new_fd = alloc_fd_since(arg);
        }

        // alloc_fd_since() is impossible to return new_fd >= MAX_FD in fact.
        if (new_fd < 0) {
                return new_fd;
        }

        type = fd_dic[fd]->type;
        fd_dic[new_fd]->fd = fd_dic[fd]->fd;
        fd_dic[new_fd]->type = fd_dic[fd]->type;
        fd_dic[new_fd]->fd_op = fd_dic[fd]->fd_op;
        if (type == FD_TYPE_SOCK || type == FD_TYPE_FILE) {
                fd_dic[new_fd]->cap = fd_dic[fd]->cap;
                fd_dic[new_fd]->flags = fd_dic[fd]->flags;
        } else if (type == FD_TYPE_STDIN || type == FD_TYPE_STDOUT
                   || type == FD_TYPE_STDERR)
                fd_dic[new_fd]->termios = fd_dic[fd]->termios;
        return new_fd;
}

/* Default file operation */

/* TYPE TO STRING */
static char fd_type[][20] = {
        "FILE",
        "PIPE",
        "SCOK",
        "STDIN",
        "STDOUT",
        "STDERR",
        "EVENT",
        "TIMER",
        "EPOLL",
};

static ssize_t fd_default_read(int fd, void *buf, size_t size)
{
        printf("READ fd %d type %s not impl!\n", fd, fd_type[fd_dic[fd]->type]);
        return -EINVAL;
}

static ssize_t fd_default_write(int fd, void *buf, size_t size)
{
        printf("WRITE fd %d type %s not impl!\n",
               fd,
               fd_type[fd_dic[fd]->type]);
        return -EINVAL;
}

static int fd_default_close(int fd)
{
        printf("CLOSE fd %d type %s not impl!\n",
               fd,
               fd_type[fd_dic[fd]->type]);
        return -EINVAL;
}

static int fd_default_poll(int fd, struct pollarg *arg)
{
        printf("POLL fd %d type %s not impl!\n", fd, fd_type[fd_dic[fd]->type]);
        return -EINVAL;
}

static int fd_default_ioctl(int fd, unsigned long request, void *arg)
{
        printf("IOCTL fd %d type %s not impl!\n",
               fd,
               fd_type[fd_dic[fd]->type]);
        return -EINVAL;
}

struct fd_ops default_ops = {
        .read = fd_default_read,
        .write = fd_default_write,
        .close = fd_default_close,
        .poll = fd_default_poll,
        .ioctl = fd_default_ioctl,
};
