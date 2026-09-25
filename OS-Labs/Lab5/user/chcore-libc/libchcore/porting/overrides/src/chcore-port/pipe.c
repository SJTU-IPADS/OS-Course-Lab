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

/* This file is depreciated. Pipe is no longer supported. */

#include <chcore/bug.h>
#include <chcore/type.h>
#include <chcore/thread.h>
#include <chcore/syscall.h>
#include <chcore/pthread.h>
#include <string.h>
#include <pthread.h>
#include <debug_lock.h>
#include "atomic.h"

#include "pipe.h"
#include "fd.h"

/* TODO: dynamic size */
#define PIPE_BUF_SIZE 1024
struct pipe_file {
        char buf[PIPE_BUF_SIZE];
        /*
         * If read_idx == write_idx, the buffer is empty.
         * Thus the maximum size is PIPE_BUF_SIZE - 1.
         */
        int read_idx;
        int write_idx;
        int volatile pipe_lock;

        int read_fd;
        int write_fd;
};

#define PIPE_RQ_READ  0
#define PIPE_RQ_WRITE 1
struct pipe_request {
        int pipe_rq_type;
        int fd;
        int count;
        char data[];
};

static long chcore_pipe_read_local(int fd, void *buf, long count);
static long chcore_pipe_write_local(int fd, void *buf, long count);

/*
 * TODO: 1. All pipe use one conn which lead to contention.
 *	 2. Pipe ops issue locally still use ipc.
 * FIXME: Closing fd in server leads to incorrrect ops issued from clients.
 */
DEFINE_SERVER_HANDLER(pipe_server_handler)
{
        struct pipe_request *pipe_rq;
        int ret;

        pipe_rq = (struct pipe_request *)ipc_get_msg_data(ipc_msg);
        switch (pipe_rq->pipe_rq_type) {
        case PIPE_RQ_READ:
                ret = chcore_pipe_read_local(
                        pipe_rq->fd, pipe_rq->data, pipe_rq->count);
                break;
        case PIPE_RQ_WRITE:
                ret = chcore_pipe_write_local(
                        pipe_rq->fd, pipe_rq->data, pipe_rq->count);
                break;
        default:
                ret = -EINVAL;
        }

        ipc_return(ipc_msg, ret);
}

volatile cap_t pipe_server_thread_cap = 0;
ipc_struct_t *pipe_ipc_struct = NULL;
volatile bool pipe_server_inited = false;

static void *init_pipe_server(void *arg)
{
        ipc_register_server(pipe_server_handler,
                            DEFAULT_CLIENT_REGISTER_HANDLER);

        pipe_ipc_struct = ipc_register_client(pipe_server_thread_cap);
        if (pipe_ipc_struct == NULL) {
                printf("pipe conn init failed\n");
                usys_exit(-1);
        }

        a_barrier();
        pipe_server_inited = true;

        while (1) {
                usys_yield();
        }

        return NULL;
}

int chcore_pipe2(int pipefd[2], int flags)
{
        int read_fd, write_fd, ret;
        struct pipe_file *pf;
        struct fd_desc *read_fd_desc, *write_fd_desc;
        pthread_t thread_id;

        /*
         * XXX: Init a thread to register pipe server. Though the thread is not
         * used after init, a thread can only register one ipc handler. So init
         * the thread to register pipe's handler.
         */
        if (unlikely(pipe_server_thread_cap == 0)) {
                pipe_server_thread_cap = chcore_pthread_create(
                        &thread_id, NULL, init_pipe_server, NULL);
                a_barrier();
                while (!pipe_server_inited)
                        usys_yield();
        }

        if (flags != 0) {
                WARN("pipe2 flags not supported");
                return -ENOSYS;
        }

        if ((pf = malloc(sizeof(*pf))) == NULL)
                return -ENOMEM;

        if ((read_fd = alloc_fd()) < 0) {
                ret = read_fd;
                goto out_free_pf;
        }

        read_fd_desc = fd_dic[read_fd];
        read_fd_desc->type = FD_TYPE_PIPE;
        read_fd_desc->private_data = pf;
        read_fd_desc->fd_op = &pipe_op;

        if ((write_fd = alloc_fd()) < 0) {
                ret = write_fd;
                goto out_free_read_fd;
        }

        write_fd_desc = fd_dic[write_fd];
        write_fd_desc->type = FD_TYPE_PIPE;
        write_fd_desc->private_data = pf;
        write_fd_desc->fd_op = &pipe_op;

        *pf = (struct pipe_file){.read_idx = 0,
                                 .write_idx = 0,
                                 .pipe_lock = 0,
                                 .read_fd = read_fd,
                                 .write_fd = write_fd};

        pipefd[0] = read_fd;
        pipefd[1] = write_fd;

        return 0;

out_free_read_fd:
        free_fd(read_fd);

out_free_pf:
        free(pf);
        return ret;
}

static long chcore_pipe_read_local(int fd, void *buf, long count)
{
        struct pipe_file *pf;
        long r, len, len_tmp;

        if (fd < 0 || fd >= MAX_FD || fd_dic[fd]->type != FD_TYPE_PIPE)
                return -EINVAL;

        pf = fd_dic[fd]->private_data;
        chcore_spin_lock(&pf->pipe_lock);
        if (pf->read_fd != fd) {
                r = -EINVAL;
                goto out_unlock;
        }

        if (pf->read_idx <= pf->write_idx) {
                len = MIN(pf->write_idx - pf->read_idx, count);
                memcpy(buf, pf->buf, len);
                pf->read_idx += len;
                r = len;
        } else {
                /* total copied length */
                len = MIN(PIPE_BUF_SIZE - pf->read_idx + pf->write_idx, count);

                /* copy from read_idx to PIPE_BUF_SIZE */
                len_tmp = MIN(PIPE_BUF_SIZE - pf->read_idx, len);
                memcpy(buf, pf->buf, len_tmp);

                /* copy from 0 to write_idx */
                memcpy(buf, pf->buf + len_tmp, len - len_tmp);

                pf->read_idx = (pf->read_idx + len) % PIPE_BUF_SIZE;
                r = len;
        }

out_unlock:
        chcore_spin_unlock(&pf->pipe_lock);
        return r;
}

static ssize_t chcore_pipe_read(int fd, void *buf, size_t count)
{
        ipc_msg_t *ipc_msg;
        struct pipe_request *pipe_rq;
        ssize_t ret;

        BUG_ON(!pipe_ipc_struct);
        ipc_msg = ipc_create_msg(
                pipe_ipc_struct, sizeof(struct pipe_request) + count);

        pipe_rq = (struct pipe_request *)ipc_get_msg_data(ipc_msg);
        pipe_rq->pipe_rq_type = PIPE_RQ_READ;
        pipe_rq->fd = fd;
        pipe_rq->count = count;

        ret = ipc_call(pipe_ipc_struct, ipc_msg);
        BUG_ON(ret < 0);

        memcpy(buf, pipe_rq->data, ret);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static long chcore_pipe_write_local(int fd, void *buf, long count)
{
        struct pipe_file *pf;
        long r, len, len_tmp;

        if (fd < 0 || fd >= MAX_FD || fd_dic[fd]->type != FD_TYPE_PIPE)
                return -EINVAL;

        pf = fd_dic[fd]->private_data;
        chcore_spin_lock(&pf->pipe_lock);
        if (pf->write_fd != fd) {
                r = -EINVAL;
                goto out_unlock;
        }

        if (pf->read_idx <= pf->write_idx) {
                /* total copied length */
                len = MIN(PIPE_BUF_SIZE + pf->read_idx - pf->write_idx - 1,
                          count);

                /* copy to write_idx to PIPE_BUF_SIZE */
                len_tmp = MIN(PIPE_BUF_SIZE - pf->write_idx, len);
                memcpy(pf->buf, buf, len_tmp);

                /* copy to 0 to read_idx */
                memcpy(pf->buf + len_tmp, buf, len - len_tmp);

                pf->write_idx = (pf->write_idx + len) % PIPE_BUF_SIZE;
                r = len;
        } else {
                len = MIN(pf->read_idx - pf->write_idx, count);
                memcpy(pf->buf, buf, len);
                pf->write_idx += len;
                r = len;
        }

out_unlock:
        chcore_spin_unlock(&pf->pipe_lock);
        return r;
}

static ssize_t chcore_pipe_write(int fd, void *buf, size_t count)
{
        ipc_msg_t *ipc_msg;
        struct pipe_request *pipe_rq;
        ssize_t ret;

        BUG_ON(!pipe_ipc_struct);
        ipc_msg = ipc_create_msg(
                pipe_ipc_struct, sizeof(struct pipe_request) + count);

        pipe_rq = (struct pipe_request *)ipc_get_msg_data(ipc_msg);
        pipe_rq->pipe_rq_type = PIPE_RQ_WRITE;
        pipe_rq->fd = fd;
        pipe_rq->count = count;
        memcpy(pipe_rq->data, buf, count);

        ret = ipc_call(pipe_ipc_struct, ipc_msg);
        BUG_ON(ret < 0);

        ipc_destroy_msg(ipc_msg);

        return ret;
}

/* TODO: How to recycle pipe server? */
static int chcore_pipe_close(int fd)
{
        return 0;
}

long chcore_pipe_llseek(int fd, long offset, int whence)
{
        return -ENOSYS;
}

/* TODO: poll, ioctl, release fasync */

/* PIPE */
struct fd_ops pipe_op = {
        .read = chcore_pipe_read,
        .write = chcore_pipe_write,
        .close = chcore_pipe_close,
        .poll = NULL,
        .ioctl = NULL,
        .fcntl = NULL,
};
