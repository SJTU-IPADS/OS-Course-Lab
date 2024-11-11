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

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "test_tools.h"

#define THREAD_NUM 32

void *client_routine(void *arg)
{
        int client_fd = 0;
        int ret = 0;
        char data_buf[NET_BUFSIZE];
        int i = 0;
        struct sockaddr_in target;
        struct msghdr msgrecv;
        struct iovec iovrecv[1];
        cpu_set_t mask;
        char server_ip[] = "127.0.0.1";
        int server_port = NET_SERVERPORT;
        int tid = ((int *)arg)[0];
        int test_num = ((int *)arg)[1];
        int offset = 0;
        int remain = 0;

        /* set aff */
        CPU_ZERO(&mask);
        CPU_SET(tid % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "Set affinity failed!");
        sched_yield();

        for (i = 0; i < test_num; i++) {
                offset = 0;
                remain = NET_BUFSIZE;
                memset(data_buf, 0, sizeof(data_buf));

                do {
                        client_fd = socket(AF_INET, SOCK_STREAM, 0);
                        if (client_fd >= 0) /* succ */
                                break;
                        error("Create socket failed! errno = %d\n", errno);
                        sleep(1);
                } while (1);

                ret = setsockopt(client_fd,
                                 SOL_SOCKET,
                                 SO_REUSEADDR,
                                 &(int){1},
                                 sizeof(int));
                chcore_assert(ret == 0, "net client: set socketopt failed");

                /* Client do not need to bind */

                memset(&target, 0, sizeof(struct sockaddr_in));
                target.sin_family = AF_INET;
                target.sin_addr.s_addr = inet_addr(server_ip);
                target.sin_port = htons(server_port);
                ret = connect(client_fd,
                              (struct sockaddr *)&target,
                              sizeof(struct sockaddr_in));
                chcore_assert(ret == 0, "net client: connect failed!");

                int mode = 0;
                ret = ioctl(client_fd, FIONBIO, &mode);
                chcore_assert(ret == 0, "net client: ioctl failed!");

                if (i % 3 == 0) {
                        while (remain > 0) {
                                ret = recv(client_fd,
                                           data_buf + offset,
                                           sizeof(data_buf) - offset,
                                           0);
                                chcore_assert(
                                        ret >= 0,
                                        "net client: recv failed!");
                                remain -= ret;
                                offset += ret;
                        }
                } else if (i % 3 == 1) {
                        while (remain > 0) {
                                ret = read(client_fd,
                                           data_buf + offset,
                                           sizeof(data_buf) - offset);
                                chcore_assert(
                                        ret >= 0,
                                        "net client: read failed!");
                                remain -= ret;
                                offset += ret;
                        }
                } else {
                        memset(&msgrecv, 0, sizeof(struct msghdr));
                        msgrecv.msg_name = 0;
                        msgrecv.msg_namelen = 0;
                        msgrecv.msg_iov = iovrecv;
                        msgrecv.msg_iovlen = 1;
                        while (remain > 0) {
                                iovrecv[0].iov_base = data_buf + offset;
                                iovrecv[0].iov_len = sizeof(data_buf) - offset;
                                /* currently we do not support large buf */
                                msgrecv.msg_flags = 0;
                                ret = recvmsg(client_fd, &msgrecv, 0);
                                chcore_assert(
                                        ret >= 0,
                                        "net client: recvmsg failed!");
                                remain -= ret;
                                offset += ret;
                        }
                }

                for (int j = 0; j < 4096; j++) {
                        chcore_assert(data_buf[j] == 'a' + j % 26,
                                      "net client: receive message check error!");
                }

                ret = shutdown(client_fd, SHUT_WR);
                chcore_assert(ret >= 0, "net client: shutdown failed!");

                ret = close(client_fd);
                chcore_assert(ret >= 0, "net client: close failed!");
        }

        return 0;
}

int main(int argc, char *argv[])
{
        pthread_t thread_id[THREAD_NUM];
        int i = 0;
        void *ret = 0;
        int thread_num = 1;
        int test_num = 20;
        int thread_args[2];

        if (argc == 2)
                thread_num = atoi(argv[1]);
        else if (argc == 3) {
                thread_num = atoi(argv[1]);
                test_num = atoi(argv[2]);
        }

        info("LWIP network test: %d threads, %d tests each thread\n",
             thread_num,
             test_num);

        for (i = 0; i < thread_num; i++) {
                thread_args[0] = i;
                thread_args[1] = test_num;
                pthread_create(&thread_id[i],
                               NULL,
                               client_routine,
                               (void *)thread_args);
        }
        for (i = 0; i < thread_num; i++) {
                pthread_join(thread_id[i], &(ret));
                chcore_assert((unsigned long)ret == 0,
                              "return value check failed!");
        }

        info("TCP test finished!\n");
        return 0;
}
