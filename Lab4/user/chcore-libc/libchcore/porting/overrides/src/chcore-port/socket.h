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

#ifndef CHCORE_PORT_SOCKET_H
#define CHCORE_PORT_SOCKET_H

#include <poll.h>
#include <sys/socket.h>

/* Socket basic operations */
int chcore_socket(int domain, int type, int protocol);
int chcore_setsocketopt(int fd, int level, int optname, const void *optval,
                        socklen_t optlen);
int chcore_getsocketopt(int fd, int level, int optname, void *restrict optval,
                        socklen_t *restrict optlen);
int chcore_getsockname(int fd, struct sockaddr *restrict addr,
                       socklen_t *restrict len);
int chcore_getpeername(int fd, struct sockaddr *restrict addr,
                       socklen_t *restrict len);
int chcore_bind(int fd, const struct sockaddr *addr, socklen_t len);
int chcore_listen(int fd, int backlog);
int chcore_accept(int fd, struct sockaddr *restrict addr,
                  socklen_t *restrict len);
int chcore_connect(int fd, const struct sockaddr *addr, socklen_t len);
int chcore_sendto(int fd, const void *buf, size_t len, int flags,
                  const struct sockaddr *addr, socklen_t alen);
int chcore_sendmsg(int fd, const struct msghdr *msg, int flags);
int chcore_recvmsg(int fd, struct msghdr *msg, int flags);
int chcore_recvfrom(int fd, void *restrict buf, size_t len, int flags,
                    struct sockaddr *restrict addr, socklen_t *restrict alen);
int chcore_shutdown(int fd, int how);

#endif /* CHCORE_PORT_SOCKET_H */
