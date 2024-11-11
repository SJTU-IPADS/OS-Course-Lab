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

int create_socket(char *target_ip)
{
        int client_socket;
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (socket < 0) {
                perror("socket");
                exit(-1);
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(NETWORK_CP_PORT);
        addr.sin_addr.s_addr = inet_addr(target_ip);


        if (connect(client_socket, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
                perror("connect");
                exit(-1);
        }

        printf("network-send-tool: connect to host %s:%d\n", target_ip, NETWORK_CP_PORT);
        return client_socket;
}

/* @file could be path/filename */
void send_file(int client_socket, char *file)
{
        int fd;
        fd = open(file, O_RDWR);
        if (fd < 0) {
                perror("open");
                exit(-1);
        }

        char *buf;
        ssize_t len;
        char *filename;

#define BUF_LEN 512
        buf = malloc(BUF_LEN);
        memset(buf, 0, BUF_LEN);

        /* extract the filename from the path */
        filename = strrchr(file, '/');
        if (filename == NULL)
                filename = file;
        else
                filename = filename + 1;

        /* send the file name first */
        len = strlen(filename);
        strncpy(buf, filename, len);
        len = send(client_socket, buf, BUF_LEN, 0);
        if (len <= 0) {
                perror("send");
                goto out;
        }
        printf("send filename: %s\n", filename);

        while (1) {
                len = read(fd, buf, BUF_LEN);
                if (len > 0) {
                        send(client_socket, buf, len, 0);
                        /* // leave for debugging
                        ssize_t ret;
                        ret = send(client_socket, buf, len, 0);
                        printf("file len: %ld, send len: %ld\n", len, ret);
                        */
                } else {
                        goto out;
                }
        }

out:
        free(buf);
        close(fd);
}

int main(int argc, char **argv)
{
        if (argc != 3) {
                printf("usage: ./%s [target IP] [local file]\n", argv[0]);
                exit(1);
        }

        int sockfd = create_socket(argv[1]);
        send_file(sockfd, argv[2]);
        close(sockfd);
        return 0;
}
