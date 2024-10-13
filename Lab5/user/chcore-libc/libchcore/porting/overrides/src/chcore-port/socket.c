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

#include "limits.h"
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syscall_arch.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>

/* ChCore header */
#include <chcore/ipc.h>
#include <chcore-internal/lwip_defs.h>
#include <chcore/syscall.h>

/* ChCore port header */
#include "fd.h"

/* Helper function */
#define lwip_ipc(x...) __lwip_ipc(NULL, x)
#define lwip_ipc_sockaddr(x... ) __lwip_ipc_sockaddr(NULL, x)

typedef unsigned char lwip_sa_family_t;

struct lwip_sockaddr_in {
	uint8_t sin_len;
	lwip_sa_family_t sin_family;
	in_port_t sin_port;
	struct in_addr sin_addr;
	uint8_t sin_zero[8];
};

struct lwip_sockaddr_in6 {
	uint8_t		sin6_len;
	lwip_sa_family_t     sin6_family;
	in_port_t       sin6_port;
	uint32_t        sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t        sin6_scope_id;
};

#define field_to_end_size(type, field) (sizeof(type) - offsetof(type, field))

static void translate_sockaddr_in_to_lwip(struct lwip_sockaddr_in *output, const struct sockaddr_in *input)
{
        output->sin_family = input->sin_family;
        /**
         * Simply copy @input->sin_port to the end of @input
         * to corresponding area of @output due to same layout
         * of these parts.
         */
        memcpy(&output->sin_port, &input->sin_port, field_to_end_size(struct sockaddr_in, sin_port));
}

static void translate_sockaddr_in6_to_lwip(struct lwip_sockaddr_in6 *output, const struct sockaddr_in6 *input)
{
        output->sin6_family = input->sin6_family;
        /**
         * Simply copy @input->sin6_port to the end of @input
         * to corresponding area of @output due to same layout
         * of these parts.
         */
        memcpy(&output->sin6_family, &input->sin6_family, field_to_end_size(struct sockaddr_in6, sin6_port));
}

static size_t translate_copy_to_lwip(const struct sockaddr *input, socklen_t input_len, void *buf)
{
        struct lwip_sockaddr_in lwip_in;
        struct lwip_sockaddr_in6 lwip_in6;
        const void *src;
        size_t src_len;

        if (input == NULL || input_len == 0) {
                return 0;
        }

        if (input->sa_family == AF_INET) {
                translate_sockaddr_in_to_lwip(&lwip_in, (struct sockaddr_in *)input);
                src = (void *)&lwip_in;
                src_len = sizeof(struct lwip_sockaddr_in);
        } else if (input->sa_family == AF_INET6) {
                translate_sockaddr_in6_to_lwip(&lwip_in6, (struct sockaddr_in6 *)input);
                src = (void *)&lwip_in6;
                src_len = sizeof(struct lwip_sockaddr_in6);
        } else {
                src = input;
                src_len = input_len;
        }

        memcpy(buf, src, src_len);
        return src_len;
}

/* Use __lwip_ipc when need to get message after IPC return */
static int __lwip_ipc(ipc_msg_t **ipc_msg_p, enum LWIP_REQ req, void *data,
                      size_t data_size, int nr_args, ...)
{
        struct lwip_request *lr_ptr;
        ipc_msg_t *ipc_msg;
        va_list args;
        int ret = 0, i = 0;

        ipc_msg =
                ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->req = req;

        va_start(args, nr_args);
        for (i = 0; i < nr_args; ++i)
                lr_ptr->args[i] = va_arg(args, unsigned long);
        va_end(args);
        if (data) {
                BUG_ON(data_size > LWIP_DATA_LEN);
                memcpy(lr_ptr->data, data, data_size);
        }
        ret = ipc_call(lwip_ipc_struct, ipc_msg);

        if (ipc_msg_p)
                *ipc_msg_p = ipc_msg;
        else
                ipc_destroy_msg(ipc_msg);
        return ret;
}

static int __lwip_ipc_sockaddr(ipc_msg_t **ipc_msg_p, enum LWIP_REQ req, const struct sockaddr *addr,
                      socklen_t len, int nr_args, ...)
{
        struct lwip_request *lr_ptr;
        ipc_msg_t *ipc_msg;
        va_list args;
        int ret = 0, i = 0;

        ipc_msg =
                ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->req = req;

        va_start(args, nr_args);
        for (i = 0; i < nr_args; ++i)
                lr_ptr->args[i] = va_arg(args, unsigned long);
        va_end(args);
        translate_copy_to_lwip(addr, len, lr_ptr->data);
        ret = ipc_call(lwip_ipc_struct, ipc_msg);

        if (ipc_msg_p)
                *ipc_msg_p = ipc_msg;
        else
                ipc_destroy_msg(ipc_msg);
        return ret;
}

/* ChCore socket fd operation (with socket prefix) */

static ssize_t chcore_socket_read(int fd, void *buf, size_t count)
{
        ipc_msg_t *ipc_msg;
        struct lwip_request *lr_ptr = 0;
        ssize_t ret = 0, len = 0, remain = 0, bias = 0;
        /**
         * read(2): 
         * On Linux, read() (and similar system calls) will transfer at most
         * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
         * actually transferred.  (This is true on both 32-bit and 64-bit
         * systems.)
         */
        count = count <= READ_SIZE_MAX ? count : READ_SIZE_MAX;

        /* fd has already been checked */
        if (count != 0 && buf == NULL)
                return -EFAULT;
        ipc_msg =
                ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->args[0] = fd_dic[fd]->conn_id;
        /* If data is too large, seperate to multiple IPC */
        if (count > LWIP_DATA_LEN) {
                remain = (ssize_t)count;
                ret = len = 0;
                lr_ptr->req = LWIP_SOCKET_RECV; /* We need to use flags */
                lr_ptr->args[2] = 0; /* flags */
                lr_ptr->args[3] = 0; /* alen */
                while (remain > 0 && ret == len) {
                        len = remain > LWIP_DATA_LEN ? LWIP_DATA_LEN : remain;
                        lr_ptr->args[1] = len;
                        if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0) {
                                ret = bias > 0 ? bias : ret;
                                goto out;
                        }
                        BUG_ON(ret > LWIP_DATA_LEN);
                        memcpy((void *)((char *)buf + bias), lr_ptr->data, ret);
                        remain -= ret;
                        bias += ret;
                        lr_ptr->args[2] |= MSG_DONTWAIT;
                }
                ret = bias;
        } else { /* else one single ipc is enough */
                lr_ptr->req = LWIP_SOCKET_READ;
                lr_ptr->args[1] = count; /* len */
                if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0)
                        goto out;
                BUG_ON(ret > LWIP_DATA_LEN);
                memcpy(buf, lr_ptr->data, ret);
        }
out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

static ssize_t chcore_socket_write(int fd, void *buf, size_t count)
{
        ipc_msg_t *ipc_msg;
        struct lwip_request *lr_ptr = 0;
        ssize_t ret = 0, len = 0, remain = 0, bias = 0;
        /**
         * see chcore_socket_read
         */
        count = count <= READ_SIZE_MAX ? count : READ_SIZE_MAX;

        if (count != 0 && buf == NULL)
                return -EFAULT;
        ipc_msg =
                ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->req = LWIP_SOCKET_WRITE;
        /* Find conn_id from fd_desc */
        lr_ptr->args[0] = fd_dic[fd]->conn_id;

        if (count > LWIP_DATA_LEN) {
                /* If data is too large, seperate to multiple IPC */
                remain = (ssize_t)count;
                len = ret = 0;
                while (remain > 0 && ret == len) {
                        /* If remain cannot send in one ipc, leave it to the
                         * next ipc */
                        len = remain > LWIP_DATA_LEN ? LWIP_DATA_LEN : remain;
                        memcpy(lr_ptr->data, (void *)((char *)buf + bias), len);
                        lr_ptr->args[1] = len; /* len */
                        if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0) {
                                ret = bias > 0 ? bias : ret;
                                goto out;
                        }
                        bias += ret;
                        remain -= ret;
                }
                ret = bias;
        } else {
                /* Else one single ipc is enough */
                memcpy(lr_ptr->data, buf, count);
                lr_ptr->args[1] = count; /* len */
                ret = ipc_call(lwip_ipc_struct, ipc_msg);
        }
out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

static int chcore_socket_close(int fd)
{
        struct lwip_request lr = {0};
        int ret;

        lr.req = LWIP_SOCKET_CLSE;
        lr.args[0] = fd_dic[fd]->conn_id;
        ret = simple_ipc_forward(lwip_ipc_struct, &lr, sizeof(lr));
        if (ret < 0)
                return ret;

        free(fd_dic[fd]->private_data);
        free_fd(fd);
        return ret;
}

static int __chcore_socket_poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
        int i, ret;
        struct pollfd *fd_iter, *server_fd_iter;
        ipc_msg_t *ipc_msg;
        struct lwip_request *lr_ptr = 0;

        if (fds == 0)
                return -EFAULT;

        if (nfds == 0)
                return 0;

        if (nfds * (sizeof(struct pollfd)) > LWIP_DATA_LEN) {
                WARN("IPC message is not large enough to store the poll fds\n");
                return -ENOMEM;
        }

        ipc_msg =
                ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);

        lr_ptr->req = LWIP_REQ_SOCKET_POLL;
        lr_ptr->args[1] = nfds;
        lr_ptr->args[2] = timeout;

        for (i = 0; i < nfds; i++) {
                server_fd_iter = ((struct pollfd *)lr_ptr->data) + i;
                /* Check fd here, only passing sock to server */
                if (fds[i].fd < MAX_FD && fds[i].fd >= 0 && /* valid fd */
                    fd_dic[fds[i].fd] != NULL
                    && fd_dic[fds[i].fd]->type == FD_TYPE_SOCK) {
                        server_fd_iter->fd = fd_dic[fds[i].fd]->conn_id;
                        server_fd_iter->events = fds[i].events;
                } else {
                        server_fd_iter->fd = -1;
                }
        }

        ret = ipc_call(lwip_ipc_struct, ipc_msg);

	/* Copy the return value */
	for (i = 0; i < nfds; i++) {
		fd_iter = fds + i;
		server_fd_iter = ((struct pollfd *)lr_ptr->data) + i;
		fd_iter->revents = server_fd_iter->revents;
	}

        ipc_destroy_msg(ipc_msg);
        return ret;
}

static int chcore_socket_poll(int fd, struct pollarg *arg)
{
	struct pollfd pfd;
	int ret;

	pfd.fd = fd;
	pfd.events = arg->events;
	pfd.revents = 0;
	ret = __chcore_socket_poll(&pfd, 1, 0);
	return ret >= 0 ? pfd.revents : 0;
}

struct socket_poll_arg {
        struct pollfd *fds;
        nfds_t nfds;
        int timeout;
        cap_t notifc_cap;
};

static int chcore_socket_ioctl(int fd, unsigned long request, void *arg)
{
        ipc_msg_t *ipc_msg;
        struct lwip_request *lr_ptr = 0;
        int ret;

        /* Only support several request in lwip */
        if (request == FIONREAD || request == FIONBIO) {
                ret = __lwip_ipc(&ipc_msg,
                                 LWIP_SOCKET_IOCTL,
                                 arg,
                                 sizeof(int),
                                 2,
                                 fd_dic[fd]->conn_id,
                                 request);
                lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
                if (ret == 0)
                        /* socket ioctl only return int in lwip */
                        memcpy(arg, lr_ptr, sizeof(int));
                ipc_destroy_msg(ipc_msg);
        } else {
                printf("Socket ioctl request %ld not supported!\n", request);
                ret = 0;
        }
        return ret;
}

int chcore_socket_fcntl(int fd, int cmd, int arg)
{
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;
        int new_fd, ret = 0;

        switch (cmd) {
        case F_DUPFD_CLOEXEC:
        case F_DUPFD: {
                new_fd = dup_fd_content(fd, arg);
                if (new_fd < 0) {
                        return new_fd;
                }
                ipc_msg = ipc_create_msg(
                        lwip_ipc_struct, sizeof(struct lwip_request));
                lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
                lr_ptr->req = LWIP_REQ_FCNTL;
                lr_ptr->args[0] = fd_dic[fd]->conn_id; /* flags */
                lr_ptr->args[1] = F_DUPFD; /* alen */
                lr_ptr->args[2] = new_fd;
                ipc_call(lwip_ipc_struct, ipc_msg);
                ipc_destroy_msg(ipc_msg);
                return new_fd;
        }
        case F_GETFL:
        case F_SETFL:
                /* SOCKET fd */
                if (fd_dic[fd]->type == FD_TYPE_SOCK) {
                        /*
                         * Remove the O_LARGEFILE bit set in fcntl.c
                         * which is not supported by LWIP.
                         */
                        arg &= ~O_LARGEFILE;
                        ipc_msg = ipc_create_msg(lwip_ipc_struct, sizeof(struct lwip_request));
                        lr_ptr = (struct lwip_request *)ipc_get_msg_data(
                                ipc_msg);
                        lr_ptr->req = LWIP_REQ_FCNTL;
                        lr_ptr->args[0] = (unsigned long)fd_dic[fd]->conn_id;
                        lr_ptr->args[1] = (unsigned long)cmd;
                        lr_ptr->args[2] = (unsigned long)arg;

                        ret = ipc_call(lwip_ipc_struct, ipc_msg);
                        ipc_destroy_msg(ipc_msg);
                        return ret;
                }

                warn("does not support F_SETFL for non-fs files\n");
                return -1;

        default:
                return -EINVAL;
        }
        return -1;
}

/* SOCK */
struct fd_ops socket_ops = {
        .read = chcore_socket_read,
        .write = chcore_socket_write,
        .close = chcore_socket_close,
        .poll = chcore_socket_poll,
        .ioctl = chcore_socket_ioctl,
        .fcntl = chcore_socket_fcntl,
};

/* ChCore Socket operation (w/o socket prefix) */

int chcore_socket(int domain, int type, int protocol)
{
        int ret = 0;
        int fd = 0;
        int *socket_type;

        ret = lwip_ipc(LWIP_CREATE_SOCKET, NULL, 0, 3, domain, type, protocol);
        if (ret >= 0) { /* succ */
                /* Allocate fd and fill the table */
                if ((fd = alloc_fd()) < 0)
                        return fd;

                fd_dic[fd]->cap = lwip_server_cap;
                fd_dic[fd]->conn_id = ret;
                fd_dic[fd]->type = FD_TYPE_SOCK;
                fd_dic[fd]->fd_op = &socket_ops;
                if (type == SOCK_DGRAM) {
                        socket_type = malloc(sizeof(int));
                        if (!socket_type) {
                                free_fd(fd);
                                return -ENOMEM;
                        }
                        *socket_type = SOCK_DGRAM;
                        fd_dic[fd]->private_data = socket_type;
                } else {
                        fd_dic[fd]->private_data = NULL;
                }
                ret = fd;
        }
        return ret;
}

int chcore_setsocketopt(int fd, int level, int optname, const void *optval,
                        socklen_t optlen)
{
        if (fd_dic[fd] == 0)
                return -EBADF;
        return lwip_ipc(LWIP_SOCKET_SOPT,
                        optlen ? (void *)optval : NULL,
                        optlen,
                        4,
                        fd_dic[fd]->conn_id,
                        level,
                        optname,
                        optlen);
}

int chcore_getsocketopt(int fd, int level, int optname, void *restrict optval,
                        socklen_t *restrict optlen)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;

        if (fd_dic[fd] == 0)
                return -EBADF;
        ret = __lwip_ipc(&ipc_msg,
                         LWIP_SOCKET_GOPT,
                         NULL,
                         0,
                         4,
                         fd_dic[fd]->conn_id,
                         level,
                         optname,
                         *(unsigned long *)optlen);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        *optlen = lr_ptr->args[3];
        BUG_ON(*optlen > LWIP_DATA_LEN);
        if (optval != 0 && *optlen != 0)
                memcpy((void *)optval, lr_ptr->data, *optlen);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_getsockname(int fd, struct sockaddr *restrict addr,
                       socklen_t *restrict len)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;

        if (fd_dic[fd] == 0)
                return -EBADF;
        ret = __lwip_ipc(&ipc_msg,
                         LWIP_SOCKET_NAME,
                         NULL,
                         0,
                         2,
                         fd_dic[fd]->conn_id,
                         *len);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        *len = lr_ptr->args[1];
        BUG_ON(*len > LWIP_DATA_LEN);
        if (addr != 0 && *len != 0)
                memcpy((void *)addr, lr_ptr->data, *len);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_getpeername(int fd, struct sockaddr *restrict addr,
                       socklen_t *restrict len)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;

        if (fd_dic[fd] == 0)
                return -EBADF;
        ret = __lwip_ipc(&ipc_msg,
                         LWIP_SOCKET_PEER,
                         NULL,
                         0,
                         2,
                         fd_dic[fd]->conn_id,
                         *len);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        *len = lr_ptr->args[1];
        BUG_ON(*len > LWIP_DATA_LEN);
        if (addr != 0 && *len != 0)
                memcpy((void *)addr, lr_ptr->data, *len);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_bind(int fd, const struct sockaddr *addr, socklen_t len)
{
        if (fd_dic[fd] == 0)
                return -EBADF;
        BUG_ON(len > LWIP_DATA_LEN);
        return lwip_ipc_sockaddr(LWIP_SOCKET_BIND,
                        addr,
                        len,
                        2,
                        fd_dic[fd]->conn_id,
                        len);
}

int chcore_listen(int fd, int backlog)
{
        if (fd_dic[fd] == 0)
                return -EBADF;
        return lwip_ipc(
                LWIP_SOCKET_LIST, NULL, 0, 2, fd_dic[fd]->conn_id, backlog);
}

int chcore_accept(int fd, struct sockaddr *restrict addr,
                  socklen_t *restrict len)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;

        if (fd_dic[fd] == 0)
                return -EBADF;

        ret = __lwip_ipc(&ipc_msg,
                         LWIP_SOCKET_ACPT,
                         NULL,
                         0,
                         2,
                         fd_dic[fd]->conn_id,
                         *len);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);

        *len = lr_ptr->args[1];
        BUG_ON(*len > LWIP_DATA_LEN);
        if (addr != 0 && *len != 0)
                memcpy((void *)addr, lr_ptr->data, *len);
        ipc_destroy_msg(ipc_msg);
        if (ret >= 0) { /* succ */
                /* Allocate fd and fill the table */
                if ((fd = alloc_fd()) < 0)
                        return fd;

                fd_dic[fd]->cap = lwip_server_cap;
                fd_dic[fd]->conn_id = ret;
                fd_dic[fd]->type = FD_TYPE_SOCK;
                fd_dic[fd]->fd_op = &socket_ops;
                ret = fd;
        }
        return ret;
}

int chcore_connect(int fd, const struct sockaddr *addr, socklen_t len)
{
        if (fd_dic[fd] == 0)
                return -EBADF;
        return lwip_ipc_sockaddr(LWIP_SOCKET_CONN,
                        addr,
                        len,
                        2,
                        fd_dic[fd]->conn_id,
                        len);
}

int chcore_sendto(int fd, const void *buf, size_t len, int flags,
                  const struct sockaddr *addr, socklen_t alen)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;
        int copylen = 0, remain = 0, bias = 0;
        cap_t shared_pmo;

        if (fd_dic[fd] == 0)
                return -EBADF;

        /* Check buf and addr cannot be NULL */
        if ((len != 0 && buf == 0) || (alen != 0 && addr == 0))
                return -EFAULT;

        /* If alen is larger than LWIP_DATA_LEN */
        BUG_ON(alen > LWIP_DATA_LEN);

        if ((len + alen > LWIP_DATA_LEN) && (fd_dic[fd]->private_data != NULL)
            && (*((int *)fd_dic[fd]->private_data) == SOCK_DGRAM))
                ipc_msg = ipc_create_msg_with_cap(
                        lwip_ipc_struct, sizeof(struct lwip_request), 1);
        else
                ipc_msg = ipc_create_msg(
                        lwip_ipc_struct, sizeof(struct lwip_request));

        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);

        lr_ptr->req = LWIP_SOCKET_SEND;
        lr_ptr->args[0] = fd_dic[fd]->conn_id;
        /* lr_ptr->args[1] (length of data) will be set later */
        lr_ptr->args[2] = flags;
        lr_ptr->args[3] = alen;

        /* data can be transmit in one IPC call*/
        if (len + alen <= LWIP_DATA_LEN) {
                lr_ptr->args[1] = len;
                memcpy(lr_ptr->data, (void *)buf, len);
                translate_copy_to_lwip(addr, alen, lr_ptr->data + len);
                ret = ipc_call(lwip_ipc_struct, ipc_msg);
                goto out;
        }

        /* If data is too large and socket type is SOCK_DGRAM, use PMO to send
         * data*/
        if ((fd_dic[fd]->private_data != NULL)
            && (*((int *)fd_dic[fd]->private_data) == SOCK_DGRAM)) {
                lr_ptr->args[1] = len;
                shared_pmo = usys_create_pmo(len + alen, PMO_DATA);
                if (shared_pmo < 0) {
                        ret = -ENOMEM;
                        goto out;
                }
                if ((ret = ipc_set_msg_cap(ipc_msg, 0, shared_pmo)) < 0)
                        goto out_revoke_pmo;

                usys_write_pmo(shared_pmo, 0, (void *)buf, len);

                if (alen != 0) {
                        if (addr->sa_family != AF_INET6 && addr->sa_family != AF_INET) {
                                usys_write_pmo(shared_pmo, len, (void *)addr, alen);
                        } else {
                                union { 
                                        struct lwip_sockaddr_in a;
                                        struct lwip_sockaddr_in6 b;
                                } sockaddr_buf;
                                size_t buflen = translate_copy_to_lwip(addr, alen, &sockaddr_buf);
                                usys_write_pmo(shared_pmo, len, (void *)&sockaddr_buf, buflen);
                        }
                }

                ret = ipc_call(lwip_ipc_struct, ipc_msg);
        }
        /* Else, separate to multiple IPC*/
        else {
                /* Init */
                copylen = 0;
                ret = 0;
                remain = len;
                /* If data is too large, seperate to multiple IPC */
                while (remain > 0 && ret == copylen) {
                        /* If remain cannot send in one ipc, leave it to the
                         * next ipc */
                        copylen = remain + alen > LWIP_DATA_LEN ?
                                          LWIP_DATA_LEN - alen :
                                          remain;
                        lr_ptr->args[1] = copylen;
                        memcpy(lr_ptr->data, (char *)buf + bias, copylen);
                        translate_copy_to_lwip(addr, alen, lr_ptr->data + copylen);
                        if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0)
                                break; /* Error occur */
                        bias += ret;
                        remain -= ret;
                }
                ret = ret < 0 ? ret : bias;
                goto out;
        }

out_revoke_pmo:
        usys_revoke_cap(shared_pmo, true);
out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_recvfrom(int fd, void *restrict buf, size_t len, int flags,
                    struct sockaddr *restrict addr, socklen_t *restrict alen)
{
        int ret = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;
        int copylen = 0, remain = len, bias = 0;
        int alen_val = (alen != 0) ? *alen : 0;
        cap_t shared_pmo;

        if (fd_dic[fd] == 0)
                return -EBADF;
        if ((len != 0 && buf == 0) || (alen != 0 && *alen != 0 && addr == 0))
                return -EFAULT;
        /* If alen is larger than LWIP_DATA_LEN */
        BUG_ON((alen != 0 && *alen > LWIP_DATA_LEN));

        if ((len + alen_val > LWIP_DATA_LEN)
            && (fd_dic[fd]->private_data != NULL)
            && (*((int *)fd_dic[fd]->private_data) == SOCK_DGRAM))
                ipc_msg = ipc_create_msg_with_cap(
                        lwip_ipc_struct, sizeof(struct lwip_request), 1);
        else
                ipc_msg = ipc_create_msg(
                        lwip_ipc_struct, sizeof(struct lwip_request));

        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);

        lr_ptr->req = LWIP_SOCKET_RECV;
        lr_ptr->args[0] = fd_dic[fd]->conn_id;
        /* lr_ptr->args[1] (length of data) will be set later */
        lr_ptr->args[2] = flags;
        lr_ptr->args[3] = alen_val;

        /* data can be transmit in one IPC call*/
        if (len + alen_val <= LWIP_DATA_LEN) {
                lr_ptr->args[1] = len;
                if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0)
                        goto out;
                BUG_ON(ret > LWIP_DATA_LEN);
                memcpy((void *)buf, lr_ptr->data, ret);
                if (alen != 0 && *alen != 0) {
                        *alen = lr_ptr->args[3];
                        memcpy((void *)addr, lr_ptr->data + len, *alen);
                }
                goto out;
        }

        /**
         * If data is too large, use PMO_DATA to transfer data (UDP package)
         * or separate to multiple IPC
         */
        if ((fd_dic[fd]->private_data != NULL)
            && (*((int *)fd_dic[fd]->private_data) == SOCK_DGRAM)) {
                lr_ptr->args[1] = len;
                shared_pmo = usys_create_pmo(len + alen_val, PMO_DATA);
                if (shared_pmo < 0) {
                        ret = -ENOMEM;
                        goto out;
                }

                if ((ret = ipc_set_msg_cap(ipc_msg, 0, shared_pmo)) < 0)
                        goto out_revoke_pmo;
                if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0)
                        goto out_revoke_pmo;

                usys_read_pmo(shared_pmo, 0, buf, len);

                if (alen != 0 && *alen != 0) {
                        *alen = lr_ptr->args[3];
                        usys_read_pmo(shared_pmo, len, addr, *alen);
                }
        }
        /* Else, separate to multiple IPC*/
        else {
                ret = copylen = 0;
                remain = len;
                while (remain > 0 && ret == copylen) {
                        copylen = remain + alen_val > LWIP_DATA_LEN ?
                                          LWIP_DATA_LEN - alen_val :
                                          remain;
                        lr_ptr->args[1] = copylen;
                        memcpy(lr_ptr->data + copylen, (void *)addr, alen_val);
                        if ((ret = ipc_call(lwip_ipc_struct, ipc_msg)) < 0) {
                                /* Already received messages? */
                                ret = bias > 0 ? bias : ret;
                                goto out;
                        }
                        BUG_ON(ret > LWIP_DATA_LEN);
                        memcpy((char *)buf + bias, lr_ptr->data, ret);
                        if (alen != 0 && *alen != 0) {
                                *alen = lr_ptr->args[3];
                                memcpy(addr, lr_ptr->data + copylen, *alen);
                        }
                        remain -= ret;
                        bias += ret;
                        lr_ptr->args[2] |= MSG_DONTWAIT; /* Update the flag */
                }
                ret = ret < 0 ? ret : bias;
                goto out;
        }

out_revoke_pmo:
        usys_revoke_cap(shared_pmo, true);
out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_sendmsg(int fd, const struct msghdr *msg, int flags)
{
        int ret = 0, len = 0, shared_pmo = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;

        if (fd_dic[fd] == 0)
                return -EBADF;

        if ((len = get_msghdr_size((struct msghdr *)msg)) < 0)
                return len;

        /* Need transfer pmo if total len > LWIP_DATA_LEN */
        ipc_msg = ipc_create_msg_with_cap(lwip_ipc_struct,
                                 sizeof(struct lwip_request),
                                 len > LWIP_DATA_LEN ? 1 : 0);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->req = LWIP_SOCKET_SMSG;
        lr_ptr->args[0] = fd_dic[fd]->conn_id;
        lr_ptr->args[1] = flags;
        lr_ptr->args[2] = len;
        if ((ret = pack_message(
                     (struct msghdr *)msg, lr_ptr->data, len, 1, &shared_pmo))
            < 0)
                goto out;
        /* Transfer the pmo cap to the server */
        if (len > LWIP_DATA_LEN) {
                ipc_set_msg_cap(ipc_msg, 0, shared_pmo);
        }
        ret = ipc_call(lwip_ipc_struct, ipc_msg);
out:
        usys_revoke_cap(shared_pmo, true);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_recvmsg(int fd, struct msghdr *msg, int flags)
{
        int ret = 0, len = 0;
        cap_t shared_pmo = 0;
        ipc_msg_t *ipc_msg = 0;
        struct lwip_request *lr_ptr;
        char *recvbuf = NULL;

        if (fd_dic[fd] == 0)
                return -EBADF;

        if ((len = get_msghdr_size((struct msghdr *)msg)) < 0)
                return len;

        ipc_msg = ipc_create_msg_with_cap(lwip_ipc_struct,
                                 sizeof(struct lwip_request),
                                 len > LWIP_DATA_LEN ? 1 : 0);
        lr_ptr = (struct lwip_request *)ipc_get_msg_data(ipc_msg);
        lr_ptr->req = LWIP_SOCKET_RMSG;
        lr_ptr->args[0] = fd_dic[fd]->conn_id;
        lr_ptr->args[1] = flags;
        lr_ptr->args[2] = len;
        if ((ret = pack_message(
                     (struct msghdr *)msg, lr_ptr->data, len, 0, &shared_pmo))
            < 0)
                goto out;

        if (len > LWIP_DATA_LEN) {
                ipc_set_msg_cap(ipc_msg, 0, shared_pmo);
                ret = ipc_call(lwip_ipc_struct, ipc_msg);
                recvbuf = malloc(len);
                usys_read_pmo(shared_pmo, 0, recvbuf, len);
                /* XXX: Directly read pmo in unpack message */
                unpack_message(msg, recvbuf);
                free(recvbuf);
        } else {
                ret = ipc_call(lwip_ipc_struct, ipc_msg);
                unpack_message(msg, lr_ptr->data);
        }
out:
        usys_revoke_cap(shared_pmo, true);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int chcore_shutdown(int fd, int how)
{
        if (fd_dic[fd] == 0)
                return -EBADF;
        return lwip_ipc(LWIP_SOCKET_STDW, NULL, 0, 2, fd_dic[fd]->conn_id, how);
}
