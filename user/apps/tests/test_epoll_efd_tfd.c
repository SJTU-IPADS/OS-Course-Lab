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

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "test_tools.h"

#define EFD_NUM         5
#define TFD_NUM         5
#define TEST_TIMES      5
#define TEST_NUM        (EFD_NUM + TFD_NUM) * TEST_TIMES

int efd[EFD_NUM];
int tfd[TFD_NUM];

void *thread_routine(void *arg)
{
        int index = (int)(long)arg, ret;
        uint64_t buf;

        for (int i = 0; i < TEST_TIMES; i++) {
                sleep(index);
                buf = 1;
                ret = write(efd[index], &buf, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t),
                        "write eventfd failed!");
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        pthread_t tid[EFD_NUM];
        uint64_t buf;
        int ret = 0, left_test_num = TEST_NUM, epoll_fd, count, idx;
        struct itimerspec new_value;
        struct epoll_event events[EFD_NUM + TFD_NUM];
        /*
         * The following arrays are to record how many times 
         * a fd has been read. They are used to validate the
         * correctness of epoll and to stop timerfds.
         */
        int efd_times[EFD_NUM] = {0}, tfd_times[TFD_NUM] = {0};

        info("Test epoll begins...\n");

        for (int i = 0; i < EFD_NUM; i++) {
                efd[i] = eventfd(0, 0);
                chcore_assert(efd[i] >= 0, "create eventfd failed!");
        }

        for (int i = 0; i < TFD_NUM; i++) {
                tfd[i] = timerfd_create(CLOCK_MONOTONIC, 0);
                chcore_assert(tfd[i] >= 0, "create timerfd failed!");
                new_value.it_value.tv_sec = i + 1;
                new_value.it_value.tv_nsec = 0;
                new_value.it_interval.tv_sec = 10;
                new_value.it_interval.tv_nsec = 0;
                timerfd_settime(tfd[i], 0, &new_value, NULL);
        }

        /* Create thread to write event fd */
        for (int i = 0; i < EFD_NUM; i++) {
                pthread_create(&tid[i], NULL, thread_routine, (void *)(long)i);
        }

        epoll_fd = epoll_create1(0);
        chcore_assert(epoll_fd > 0, "create epollfd failed!");

        for (int i = 0; i < EFD_NUM; i++) {
                events[i].data.u32 = i;
                events[i].events = EPOLLIN;
                ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                                efd[i], &events[i]);
                chcore_assert(ret == 0, "epoll_ctl failed!");
        }

        for (int i = 0; i < TFD_NUM; i++) {
                events[i + EFD_NUM].data.u32 = i + EFD_NUM;
                events[i + EFD_NUM].events = EPOLLIN;
                ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                                tfd[i], &events[i + EFD_NUM]);
                chcore_assert(ret == 0, "epoll_ctl failed!");
        }

        new_value.it_value.tv_sec = 0;
        new_value.it_value.tv_nsec = 0;
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;

        while (left_test_num > 0) {
                count = epoll_wait(epoll_fd, events, EFD_NUM + TFD_NUM, -1);
                chcore_assert(count >= 0, "poll failed");

                for (int i = 0; i < count; i++) {
                        idx = events[i].data.u32;
                        if ((events[i].events & EPOLLIN) && idx < EFD_NUM) {
                                info("read eventfd %d\n", idx);
                                ret = read(efd[idx], &buf, sizeof(uint64_t));
                                chcore_assert(ret == sizeof(uint64_t),
                                        "read eventfd failed!");
                                chcore_assert(buf > 0,
                                        "check eventfd read value failed!");
                                efd_times[idx] += (int)buf;
                                left_test_num -= (int)buf;
                        } else if ((events[i].events & EPOLLIN) &&
                                   idx < EFD_NUM + TFD_NUM) {
                                info("read timerfd %d\n", idx - EFD_NUM);
                                ret = read(tfd[idx - EFD_NUM], &buf,
                                           sizeof(uint64_t));
                                chcore_assert(ret == sizeof(uint64_t),
                                        "read timerfd failed!");
                                /* Disarm timerfds after having read enough times. */
                                if (++tfd_times[idx - EFD_NUM] == TEST_TIMES) {
                                        timerfd_settime(tfd[idx - EFD_NUM], 0,
                                                        &new_value, NULL);
                                }
                                left_test_num--;
                        } else {
                                error("Invalid event data!");
                        }
                }
        }

        for (int i = 0; i < EFD_NUM; i++) {
                chcore_assert(efd_times[i] == TEST_TIMES,
                        "eventfd hit times check failed!");
        }
        for (int i = 0; i < TFD_NUM; i++) {
                chcore_assert(tfd_times[i] == TEST_TIMES,
                        "timerfd hit times check failed!");
        }

        green_info("Test epoll finished\n");

        return 0;
}
