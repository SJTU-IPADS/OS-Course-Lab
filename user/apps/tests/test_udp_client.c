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
#include "test_tools.h"

#define SERVER_PORT 9101
#define CLIENT_PORT 9102

#define BUFSIZE 8192

void send_one_udp_packet(int socket, char *buffer, int send_len,
                         struct sockaddr *addr, socklen_t alen)
{
        int i, send_size;

        memset(buffer, 0, send_len);
        for (i = 0; i < send_len; ++i)
                buffer[i] = 'a' + i % 26;

        send_size = sendto(socket, buffer, send_len, 0, addr, alen);
        chcore_assert(send_size == send_len, "udp client send failed!");
        sleep(0.5);
}

int main(int argc, char *argv[])
{
        char buf[BUFSIZE];
        int ret;
        int sock;

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        chcore_assert(sock >= 0, "udp client: socket failed!");

        struct sockaddr_in serveraddr;
        struct sockaddr_in clientaddr;
        socklen_t serveraddrlen = sizeof(serveraddr);
        char server_ip[] = "127.0.0.1";

        bzero(&serveraddr, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr(server_ip);
        serveraddr.sin_port = htons(SERVER_PORT);

        bzero(&clientaddr, sizeof(clientaddr));
        clientaddr.sin_family = AF_INET;
        clientaddr.sin_port = htons(CLIENT_PORT);
        clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);

        ret = bind(sock, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
        chcore_assert(ret >= 0, "udp client: bind failed!");

        send_one_udp_packet(
                sock, buf, 512, (struct sockaddr *)&serveraddr, serveraddrlen);
        send_one_udp_packet(
                sock, buf, 256, (struct sockaddr *)&serveraddr, serveraddrlen);
        send_one_udp_packet(
                sock, buf, 4096, (struct sockaddr *)&serveraddr, serveraddrlen);

        send_one_udp_packet(
                sock, buf, 4096, (struct sockaddr *)&serveraddr, serveraddrlen);
        send_one_udp_packet(
                sock, buf, 512, (struct sockaddr *)&serveraddr, serveraddrlen);
        send_one_udp_packet(
                sock, buf, 8192, (struct sockaddr *)&serveraddr, serveraddrlen);
        send_one_udp_packet(
                sock, buf, 4096, (struct sockaddr *)&serveraddr, serveraddrlen);

        ret = close(sock);
        chcore_assert(ret >= 0, "udp client: close failed!");

        info("UDP test finished!\n");

        return 0;
}
