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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <chcore/launcher.h>

#include "test_tools.h"

#define PORT    9101
#define BUFSIZE 8192

void receive_one_udp_packet(int socket, char *buffer, int recv_len,
                            int expected_len)
{
        int i, recv_size;

        recv_size = recv(socket, buffer, recv_len, 0);
        chcore_assert((recv_size == expected_len), "udp server: recv failed!");

        for (i = 0; i < recv_size; ++i)
                chcore_assert(buffer[i] == 'a' + i % 26,
                              "udp server: receive message check error!");
}

int main(int argc, char *argv[])
{
        int ret;
        int sock;
        char buf[BUFSIZE];
        struct sockaddr_in serveraddr;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        chcore_assert(sock >= 0, "udp server: socket failed!");
        ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
        chcore_assert(ret == 0, "udp server: setsockopt failed!");

        bzero(&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serveraddr.sin_port = htons(PORT);

        ret = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        chcore_assert(ret >= 0, "udp server: bind failed!");
        info("UDP test server is up!\n");

        argv[0] = "/test_udp_client.bin";
        argc = 1;
        ret = create_process(argc, argv, NULL);
        chcore_assert(ret >= 0, "create process %s failed!", argv[0]);

        receive_one_udp_packet(sock, buf, 512, 512);
        receive_one_udp_packet(sock, buf, 512, 256);
        receive_one_udp_packet(sock, buf, 512, 512);

        receive_one_udp_packet(sock, buf, 4096, 4096);
        receive_one_udp_packet(sock, buf, 4096, 512);
        receive_one_udp_packet(sock, buf, 4096, 4096);
        receive_one_udp_packet(sock, buf, 8192, 4096);

        ret = close(sock);
        chcore_assert(ret >= 0, "udp server: close failed!");

        return 0;
}
