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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 9000

void *handle_client(void *arg)
{
        int client_sock = (int)(long)arg;
        char buf[4096];

        printf("Serving it\n");
        while (1) {
                ssize_t size = recv(client_sock, buf, sizeof(buf), 0);
                if (size <= 0)
                        break;
                printf("Data come:\n");
                for (int i = 0; i < size; i++) {
                        putchar(buf[i]);
                }
                putchar('\n');
                send(client_sock, buf, size, 0);
        }
        printf("Shutdown\n");
        shutdown(client_sock, SHUT_RDWR);
        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(PORT);
        bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
        listen(sock, 10);
        printf("TCP Server is running at 0.0.0.0:%d\n", PORT);

        do {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len;
                int client_sock = accept(sock,
                                         (struct sockaddr *)&client_addr,
                                         &client_addr_len);
                printf("A client come\n");

                pthread_t tid;
                pthread_create(
                        &tid, NULL, handle_client, (void *)(long)client_sock);
        } while (1);

        close(sock);
        return 0;
}
