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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sched.h>

#include "test_tools.h"

#define MAX_CLIENT 64
#define SEND_COUNT 10

int main(int argc, char *argv[], char *envp[])
{
        int client_fd = 0;
        int ret = 0;
        char data_buf[256];
        int i, j;
        int port;
        struct sockaddr_in target;
        int fds[MAX_CLIENT];

        if (argc < 2) {
                info("Usage: %s [server port]\n", argv[0]);
                return -1;
        }
        port = atoi(argv[1]);

        for (i = 0; i < MAX_CLIENT; i++) {
                client_fd = socket(AF_INET, SOCK_STREAM, 0);
                chcore_assert(client_fd >= 0,
                              "poll client: create socket failed\n");

                memset(&target, 0, sizeof(struct sockaddr_in));
                target.sin_family = AF_INET;
                target.sin_addr.s_addr = inet_addr("127.0.0.1");
                target.sin_port = htons(port);
                ret = connect(client_fd,
                              (struct sockaddr *)&target,
                              sizeof(struct sockaddr_in));
                chcore_assert(ret >= 0, "poll client: connect failed!");

                fds[i] = client_fd;
        }

        for (i = 0; i < SEND_COUNT; i++) {
                usleep(1000);
                for (j = 0; j < MAX_CLIENT; j++) {
                        usleep(1000);
                        sprintf(data_buf,
                                "msg from client:%d count:%d!\n",
                                j,
                                i);
                        ret = send(fds[j], data_buf, strlen(data_buf), 0);
                        chcore_assert(ret >= 0, "poll client: send failed!");
                }
        }

        for (i = 0; i < MAX_CLIENT; i++) {
                ret = shutdown(fds[i], SHUT_WR);
                chcore_assert(ret >= 0, "poll client: shutdown failed!");

                ret = close(fds[i]);
                chcore_assert(ret >= 0, "poll client: close failed!");
        }

        info("LWIP poll client finished!\n");
        return 0;
}
