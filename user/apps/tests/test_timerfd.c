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

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "test_tools.h"

static int set_timerfd(int timerfd, int init_sec, int init_nsec,
                       int inter_sec, int flag)
{
        struct itimerspec new_value;

        new_value.it_value.tv_sec = init_sec;
        new_value.it_value.tv_nsec = init_nsec;
        new_value.it_interval.tv_sec = inter_sec;
        new_value.it_interval.tv_nsec = 0;
        return timerfd_settime(timerfd, flag, &new_value, NULL);
}

int main()
{
        int timerfd, timerfd_nb, ret;
        struct timespec time;
        uint64_t counter = 0;

        info("Test timerfd begins...\n");

        timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
        chcore_assert(timerfd >= 0, "timerfd_create failed!");

        /*
         * Set the timerfd to let the initial expiration
         * happens 4 seconds later, and then repeats every 1 second.
         */
        ret = set_timerfd(timerfd, 4, 0, 1, 0);
        chcore_assert(ret == 0, "timerfd_settime failed!");

        /* Read 8s later, should receive at least 5 expirations. */
        sleep(8);
        ret = read(timerfd, &counter, sizeof(counter));
        chcore_assert(ret == sizeof(counter), "read failed!");
        chcore_assert(counter >= 5, "wrong expiration count, "
                      "should be 5 but get %" PRIu64, counter);

        /* Read 3s later, should receive at least 3 expirations. */
        sleep(3);
        ret = read(timerfd, &counter, sizeof(counter));
        chcore_assert(ret == sizeof(counter), "read failed!");
        chcore_assert(counter >= 3, "wrong expiration count, "
                      "should be 3 but get %" PRIu64, counter);

        /* Set a new timer and read it immediately. */
        ret = set_timerfd(timerfd, 4, 0, 1, 0);
        chcore_assert(ret == 0, "timerfd_settime failed!");
        /* Should be blocked until the initial expiration. */
        ret = read(timerfd, &counter, sizeof(counter));
        chcore_assert(ret == sizeof(counter), "read failed!");
        chcore_assert(counter >= 1, "wrong expiration count, "
                      "should be 1 but get %" PRIu64, counter);

        ret = close(timerfd);
        chcore_assert(ret == 0, "close failed!");

        /* Test none-blocking timerfd */
        timerfd_nb = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        chcore_assert(timerfd_nb >= 0, "timerfd_create failed!");

        /*
         * Test TFD_TIMER_ABSTIME mode, thus the initial
         * expiration happens 4 seconds later, and repeats
         * every 1 second.
         */
        clock_gettime(CLOCK_MONOTONIC, &time);
        ret = set_timerfd(timerfd_nb, 4 + time.tv_sec,
                          time.tv_nsec, 1, TFD_TIMER_ABSTIME);
        chcore_assert(ret == 0, "timerfd_settime failed!");

        /*
         * Read the none-blocking timerfd immediately.
         * The read should not be blocked and should return an error.
         */
        ret = read(timerfd_nb, &counter, sizeof(counter));
        chcore_assert(ret < 0 && errno == EAGAIN,
                "read none-block timerfd status wrong!");

        /* Read 8s later, should receive at least 5 expirations. */
        sleep(8);
        ret = read(timerfd_nb, &counter, sizeof(counter));
        chcore_assert(ret == sizeof(counter), "read failed!");
        chcore_assert(counter >= 5, "wrong expiration count, "
                      "should be 5 but get %" PRIu64, counter);

        ret = close(timerfd_nb);
        chcore_assert(ret == 0, "close failed!");

        green_info("Test timerfd finished!\n");

        return 0;
}
