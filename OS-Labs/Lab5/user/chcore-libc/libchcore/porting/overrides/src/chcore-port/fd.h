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

#ifndef CHCORE_PORT_FD_H
#define CHCORE_PORT_FD_H

#include "chcore/container/hashtable.h"
#include "chcore/ipc.h"
#include <termios.h>
#include <chcore-internal/lwip_defs.h>

#include "poll.h"

#define MAX_FD 1024
#define MIN_FD 0

#define warn(fmt, ...) printf("[WARN] " fmt, ##__VA_ARGS__)
/* Type of fd */
enum fd_type {
        FD_TYPE_FILE = 0,
        FD_TYPE_PIPE,
        FD_TYPE_SOCK,
        FD_TYPE_STDIN,
        FD_TYPE_STDOUT,
        FD_TYPE_STDERR,
        FD_TYPE_EVENT,
        FD_TYPE_TIMER,
        FD_TYPE_EPOLL,
};

struct fd_ops {
        ssize_t (*read)(int fd, void *buf, size_t count);
        ssize_t (*write)(int fd, void *buf, size_t count);
        ssize_t (*pread)(int fd, void *buf, size_t count, off_t offset);
        ssize_t (*pwrite)(int fd, void *buf, size_t count, off_t offset);
        int (*close)(int fd);
        int (*poll)(int fd, struct pollarg *arg);
        int (*ioctl)(int fd, unsigned long request, void *arg);
        int (*fcntl)(int fd, int cmd, int arg);
};

extern struct fd_ops epoll_ops;
extern struct fd_ops socket_ops;
extern struct fd_ops file_ops;
extern struct fd_ops event_op;
extern struct fd_ops timer_op;
extern struct fd_ops pipe_op;
extern struct fd_ops stdin_ops;
extern struct fd_ops stdout_ops;
extern struct fd_ops stderr_ops;

/*
 * Each fd will have a fd structure `fd_desc` which can be found from
 * the `fd_dic`. `fd_desc` structure contains the basic information of
 * the fd.
 */
struct fd_desc {
        /* Identification used by corresponding service */
        union {
                int conn_id;
                int fd;
        };
        /* Baisc informantion of fd */
        int flags; /* Flags of the file */
        cap_t cap; /* Service's cap of fd, 0 if no service */
        enum fd_type type; /* Type for debug use */
        struct fd_ops *fd_op;

        /* stored termios */
        struct termios termios;

        /* Private data of fd */
        void *private_data;
};

extern struct fd_desc *fd_dic[MAX_FD];

/* fd */
int alloc_fd(void);
int alloc_fd_since(int min);
void free_fd(int fd);

/* fd operation */
ssize_t chcore_read(int fd, void *buf, size_t count);
ssize_t chcore_write(int fd, void *buf, size_t count);
ssize_t chcore_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t chcore_pwrite(int fd, void *buf, size_t count, off_t offset);
int chcore_close(int fd);
int chcore_ioctl(int fd, unsigned long request, void *arg);
ssize_t chcore_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t chcore_writev(int fd, const struct iovec *iov, int iovcnt);
int dup_fd_content(int fd, int arg);

#endif /* CHCORE_PORT_FD_H */
