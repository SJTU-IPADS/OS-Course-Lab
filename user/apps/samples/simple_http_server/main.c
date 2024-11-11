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
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <chcore/container/list.h>

#include "request.h"
#include "header.h"

static const char *ext_mime_mapping[][2] = {
        {"html", "text/html"},
        {"txt", "text/plain"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"png", "image/png"},
        {"gif", "image/gif"},
        {"bmp", "image/bmp"},
};

const char *get_mime_type(const char *ext)
{
        for (int i = 0;
             i < sizeof(ext_mime_mapping) / sizeof(*ext_mime_mapping);
             i++) {
                if (0 == strcmp(ext_mime_mapping[i][0], ext)) {
                        return ext_mime_mapping[i][1];
                }
        }
        return NULL;
}

#define PORT    8080
#define SERVER  "ChCoreSimpleHttpServer"
#define BUF_LEN 4096

static const char *RESPONSE_HEAD_TMPL = "\
HTTP/1.1 %d %s\r\n\
Server: " SERVER "\r\n\
Content-Type: %s\r\n\
Content-Length: %d\r\n\
\r\n\
";

void handle_client(int sock)
{
        char buf[BUF_LEN];

        // printf("Serving it\n");
        ssize_t size = recv(sock, buf, sizeof(buf), 0);
        if (size <= 0) {
                goto down;
        }

        struct request *request = request_parse(buf);
        if (!request || request->method != HTTP_GET) {
                // TODO: too simple
                printf("Bad request\n");
                goto down;
        }

        printf("Request: GET %s\n", request->path);

        if (request->path[0] != '/') {
                printf("Bad request\n");
                goto down;
        }

        char filename[200] = {'\0'};
        strcat(filename, "/www/docs"); // TODO(qyc): pass in through argv
        strcat(filename, request->path);
        if (request->path[strlen(request->path) - 1] == '/') {
                strcat(filename, "index.html");
        }

        const char *ext = &filename[strlen(filename)];
        for (; ext > filename && *(ext - 1) != '.'; ext--)
                ;
        const char *mime = get_mime_type(ext);
        mime = mime ? mime : "application/octet-stream";

        struct stat st;
        int fd = -1;
        if (stat(filename, &st) >= 0 && (fd = open(filename, O_RDONLY)) >= 0) {
                sprintf(buf,
                        RESPONSE_HEAD_TMPL,
                        200,
                        "OK",
                        mime,
                        (int)st.st_size);
                send(sock, buf, strlen(buf), 0);
                while ((size = read(fd, buf, BUF_LEN)) > 0) {
                        send(sock, buf, size, 0);
                }
        } else {
                // no such file
                const char *html = "<h1>404 Not Found</h1>";
                sprintf(buf,
                        RESPONSE_HEAD_TMPL,
                        404,
                        "Not Found",
                        "text/html",
                        strlen(html));
                send(sock, buf, strlen(buf), 0);
                send(sock, html, strlen(html), 0);
        }
        if (fd >= 0) {
                close(fd);
        }

down:
        // printf("Shutdown\n");
        shutdown(sock, SHUT_RDWR);
}

#define THREAD_POOL_SIZE 4

static pthread_t workers[THREAD_POOL_SIZE];

struct client {
        int sock;
        struct list_head queue_node;
};

static struct list_head queue;
static pthread_mutex_t queue_lock;
static pthread_cond_t queue_cond;

void *worker(void *arg)
{
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET((unsigned long)arg % 4, &mask);
        sched_setaffinity(0, sizeof(mask), &mask);
        sched_yield();

        int client_sock;
        for (;;) {
                pthread_mutex_lock(&queue_lock);
                while (list_empty(&queue)) {
                        pthread_cond_wait(&queue_cond, &queue_lock);
                }
                struct client *client =
                        container_of(queue.next, struct client, queue_node);
                list_del(&client->queue_node);
                pthread_mutex_unlock(&queue_lock);

                client_sock = client->sock;
                free(client);

                handle_client(client_sock);
        }
        return NULL;
}

void submit_client(int sock)
{
        struct client *client = (struct client *)malloc(sizeof(struct client));
        client->sock = sock;
        init_list_head(&client->queue_node);

        pthread_mutex_lock(&queue_lock);
        list_append(&client->queue_node, &queue);
        pthread_mutex_unlock(&queue_lock);
        pthread_cond_signal(&queue_cond);
}

void run_server()
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
        printf("Server is running at 0.0.0.0:%d\n", PORT);

        do {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len;
                int client_sock = accept(sock,
                                         (struct sockaddr *)&client_addr,
                                         &client_addr_len);
                // printf("A client come\n");

                submit_client(client_sock);
                // handle_client(client_sock);
        } while (1);

        close(sock);
}

int main(int argc, char *argv[], char *envp[])
{
        pthread_mutex_init(&queue_lock, NULL);
        pthread_cond_init(&queue_cond, NULL);
        init_list_head(&queue);

        for (int i = 0; i < THREAD_POOL_SIZE; i++)
                pthread_create(&workers[i], NULL, worker, NULL);

        run_server();

        for (int i = 0; i < THREAD_POOL_SIZE; i++)
                pthread_join(workers[i], NULL);
        return 0;
}
