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
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <poll.h>
#include <errno.h>

#include "test_tools.h"

/*
 * This file generates poll_server.bin and epoll_server.bin.
 * Use poll by default unless USE_EPOLL is defined.
 */
#ifdef USE_EPOLL
#define LOGFILE_PATH "test_epoll.log"
#else
#define USE_POLL
#define LOGFILE_PATH "test_poll.log"
#endif

#define MAX_CLIENT 64

#ifdef USE_POLL
/* Handle fds get by poll. Returns number of fd closed */
static int handle_poll_fds(struct pollfd *pfds, int count)
{
        int ret;
        const int len = 256;
        char data_buf[len];
        int closed_count = 0;

        for (int i = 0; i < MAX_CLIENT; i++) {
                if ((pfds[i].revents & POLLIN) != 0) {
                        ret = recv(pfds[i].fd, data_buf, len, 0);
                        data_buf[ret] = 0;
                        chcore_assert(ret >= 0, "recv failed");
                        if (ret > 0) {
                                info("poll server receive:\n%s", data_buf);
                        } else {
                                close(pfds[i].fd);
                                closed_count++;
                        }
                }
        }
        return closed_count;
}
#endif

#ifdef USE_EPOLL
/* Handle events get by epoll. Returns number of fd closed */
static int handle_poll_events(struct epoll_event *events, int count)
{
        int ret;
        const int len = 256;
        char data_buf[len];
        int closed_count = 0;

        for (int i = 0; i < count; i++) {
                if ((events[i].events & POLLIN) != 0) {
                        ret = recv(events[i].data.fd, data_buf, len, 0);
                        data_buf[ret] = 0;
                        chcore_assert(ret >= 0, "recv failed!");
                        if (ret > 0) {
                                info("epoll server receive:\n%s", data_buf);
                        } else {
                                close(events[i].data.fd);
                                closed_count++;
                        }
                }
        }
        return closed_count;
}
#endif

const int sleep_time[] = {-1, 0, 1, 1, 50, 200};
static int idx_to_time(int idx)
{
        return sleep_time[idx % (sizeof(sleep_time) / sizeof(sleep_time[0]))];
}

int main(int argc, char *argv[], char *envp[])
{
        int server_fd = 0, accept_fd = 0;
        int ret = 0, i;
        struct sockaddr_in sa, ac;
        socklen_t socklen;
        int closed_count;
        int event_count;
        FILE *file;
        const char *info_content = "LWIP server up!";
#ifdef USE_EPOLL
        const char *poll_method = "epoll";
        struct epoll_event event, events[MAX_CLIENT];
        int epoll_fd = epoll_create1(0);
        chcore_assert(epoll_fd >= 0, "epoll_create1 failed!");
#elif defined(USE_POLL)
        const char *poll_method = "poll";
        struct pollfd pfds[MAX_CLIENT];
#endif

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        chcore_assert(ret >= 0, "poll server: create socket failed!");

        memset(&sa, 0, sizeof(struct sockaddr));
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = inet_addr("127.0.0.1");
#ifdef USE_EPOLL
        sa.sin_port = htons(9001);
#elif defined(USE_POLL)
        sa.sin_port = htons(9002);
#endif
        ret = bind(
                server_fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in));
        chcore_assert(ret >= 0, "poll server: bind failed!");

        ret = listen(server_fd, MAX_CLIENT);
        chcore_assert(ret >= 0, "poll server: listen failed!");

        info("LWIP %s server is up!\n", poll_method);
        file = fopen(LOGFILE_PATH, "a");
        fwrite(info_content, strlen(info_content) + 1, 1, file);
        fclose(file);

        socklen = sizeof(struct sockaddr_in);
        for (i = 0; i < MAX_CLIENT; i++) {
                accept_fd = accept(server_fd, (struct sockaddr *)&ac, &socklen);
                chcore_assert(accept_fd >= 0, "poll server: accept failed!");
#ifdef USE_EPOLL
                event.events = EPOLLIN;
                event.data.fd = accept_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &event)) {
                        error("Failed to add fd to epoll\n");
                        close(epoll_fd);
                        return 1;
                }
#elif defined(USE_POLL)
                pfds[i].fd = accept_fd;
                pfds[i].events = POLLIN | POLLERR;
#endif
        }

        closed_count = 0;
        i = 0;
        while (closed_count != MAX_CLIENT) {
                i++;
#ifdef USE_EPOLL
                event_count = epoll_wait(
                        epoll_fd, events, MAX_CLIENT, idx_to_time(i));
#elif defined(USE_POLL)
                event_count = poll(pfds, MAX_CLIENT, idx_to_time(i));
#endif
                chcore_assert(event_count >= 0, "poll failed!");
                if (event_count == 0) {
                        continue;
                }

#ifdef USE_EPOLL
                closed_count += handle_poll_events(events, event_count);
#elif defined(USE_POLL)
                closed_count += handle_poll_fds(pfds, event_count);
#endif
                chcore_assert(closed_count <= MAX_CLIENT, "wrong closed count!");
        }

        ret = close(server_fd);
        chcore_assert(ret >= 0, "poll server: close failed!");

        info("LWIP %s server finished!\n", poll_method);

        return 0;
}
