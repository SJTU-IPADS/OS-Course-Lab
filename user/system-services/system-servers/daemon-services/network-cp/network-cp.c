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
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define NETWORK_CP_PORT 4096

int create_socket(void)
{
        int listen_socket;
        listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_socket < 0) {
                perror("socket");
                exit(-1);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(NETWORK_CP_PORT);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        int ret = bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr));
        if (ret < 0) {
                perror("bind");
                exit(-1);
        }

        ret = listen(listen_socket, 5);
        if (ret < 0) {
                perror("listen");
                exit(-1);
        }

        printf("Network-CP-Daemon: running at localhost:%d\n", NETWORK_CP_PORT);
        return listen_socket;
}

void handle_client(int client_socket)
{
        char *buf;
        ssize_t len;
        int fd;

#define BUF_LEN 512
        buf = malloc(BUF_LEN);
        if (buf == NULL) {
                printf("network-cp: handle_client malloc out of memory\n");
                return;
        }

        /* Recv the file name first */
        len = recv(client_socket, buf, BUF_LEN, 0);
        if (len <= 0) {
                perror("recv");
                goto free_buf;
        }

        printf("Network-CP-Daemon: prepare to recv file %s\n", buf);
        // TODO: set a proper file permission
        fd = open(buf, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        if (fd < 0) {
                perror("open");
                goto free_buf;
        }

        while (1) {
                len = recv(client_socket, buf, BUF_LEN, 0);
                if (len > 0) {
                        write(fd, buf, len);
                } else {
                        close(fd);
                        break;
                }
        }
        printf("Network-CP-Daemon: recv finished\n");

free_buf:
        free(buf);
}

int run_service(void)
{
        int listen_socket = create_socket();

        while (1) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len;
                int client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_len);
                if (client_socket < 0) {
                        perror("accept");
                        continue;
                }
                printf("Network-CP-Daemon: build one connection\n");
                handle_client(client_socket);
        }
}

int main(void)
{
        run_service();
        return 0;
}
