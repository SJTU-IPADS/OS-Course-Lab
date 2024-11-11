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
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "test_tools.h"

#define LOGFILE_PATH "test_net.log"

void *handle_thread(void *arg)
{
        char large_buffer[NET_BUFSIZE];
        struct iovec iosend[1];
        struct sockaddr_in client_addr;
        struct msghdr msgsend;
        int accept_fd = (int)(long)arg;
        socklen_t len;
        char IP[16];
        int ret = 0;
        // unsigned int port;
        cpu_set_t mask;

        /* set aff */
        CPU_ZERO(&mask);
        CPU_SET(accept_fd % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "Set affinity failed!");
        sched_yield();

        bzero(&client_addr, sizeof(client_addr));
        len = sizeof(client_addr);
        ret = getpeername(accept_fd, (struct sockaddr *)&client_addr, &len);
        chcore_assert(ret >= 0, "net server: getpeername failed!");

        inet_ntop(AF_INET, &client_addr.sin_addr, IP, sizeof(IP));
        // port = ntohs(client_addr.sin_port);

        for (int i = 0; i < sizeof(large_buffer); i++) {
                large_buffer[i] = 'a' + i % 26;
        }

        if (accept_fd % 5 == 1) { /* Use send msg */
                memset(&msgsend, 0, sizeof(msgsend));
                memset(iosend, 0, sizeof(iosend));
                msgsend.msg_name = &client_addr;
                msgsend.msg_namelen = sizeof(client_addr);

                msgsend.msg_iov = iosend;
                msgsend.msg_iovlen = 1;
                iosend[0].iov_base = large_buffer;
                iosend[0].iov_len = sizeof(large_buffer);
                ret = sendmsg(accept_fd, &msgsend, 0);
                chcore_assert(ret == sizeof(large_buffer),
                              "net server: sendmsg failed!");
        } else if (accept_fd % 5 == 2) {
                ret = send(accept_fd, large_buffer, sizeof(large_buffer), 0);
                chcore_assert(ret == sizeof(large_buffer),
                              "net server: send failed!");

        } else {
                ret = write(accept_fd, large_buffer, sizeof(large_buffer));
                chcore_assert(ret == sizeof(large_buffer),
                              "net server: write failed!");
        }

        ret = shutdown(accept_fd, SHUT_RDWR);
        chcore_assert(ret == 0, "net server: shutdown failed!");

        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int server_fd = 0;
        int ret = 0;
        int accept_fd = 0;
        struct sockaddr_in my_addr;
        struct sockaddr_in sa, ac;
        socklen_t len;
        int type = 0;
        cpu_set_t mask;
        int test_num = 20;
        int thread_num = 1;
        int test_cnt = 0;
        int i;
        const char *info_content = "LWIP tcp server up!";

        if (argc == 2) {
                thread_num = atoi(argv[1]);
        } else if (argc == 3) {
                thread_num = atoi(argv[1]);
                test_num = atoi(argv[2]);
        }

        int total_num = test_num * thread_num;
        pthread_t tid[total_num];
        /* set aff */
        CPU_ZERO(&mask);
        CPU_SET(2 % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "Set affinity failed!");
        sched_yield();

        do {
                server_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (server_fd >= 0) /* succ */
                        break;
                error("Create socket failed! errno = %d\n", errno);
                sched_yield();
        } while (1);

        ret = setsockopt(
                server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
        chcore_assert(ret >= 0, "net server: setsockopt failed!");

        len = sizeof(int);
        ret = getsockopt(server_fd, SOL_SOCKET, SO_TYPE, &type, &len);
        chcore_assert(ret >= 0 && type == SOCK_STREAM,
                      "net server: getsockopt failed!");

        memset(&sa, 0, sizeof(struct sockaddr));
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = INADDR_ANY;
        sa.sin_port = htons(NET_SERVERPORT);
        ret = bind(
                server_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
        chcore_assert(ret >= 0, "net server: bind failed!");

        ret = listen(server_fd, 5);
        chcore_assert(ret >= 0, "net server: listen failed!");

        bzero(&my_addr, sizeof(my_addr));
        len = sizeof(my_addr);

        ret = getsockname(server_fd, (struct sockaddr *)&my_addr, &len);
        chcore_assert(ret >= 0, "net server: getsockname failed!");

        info("TCP test server is up!\n");
        write_log(LOGFILE_PATH, info_content);

        do {
                len = sizeof(struct sockaddr_in);

                do {
                        accept_fd =
                                accept(server_fd, (struct sockaddr *)&ac, &len);
                        if (accept_fd >= 0) { /* succ */
                                test_cnt++;
                                break;
                        }
                        error("Accept socket failed! errno = %d\n", errno);
                        sched_yield();
                } while (1);
                pthread_create(&tid[test_cnt - 1],
                               NULL,
                               handle_thread,
                               (void *)(long)accept_fd);
        } while (test_cnt < total_num);
        for (i = 0; i < total_num; i++) {
                pthread_join(tid[i], NULL);
        }
        ret = close(server_fd);
        chcore_assert(ret >= 0, "net server: close failed!");

        return 0;
}
