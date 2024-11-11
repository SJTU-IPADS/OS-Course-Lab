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

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "test_tools.h"

#define WRITEVAL        2
#define WRITECNT        100000
#define THREAD_NUM      20

int efd1, efd2, efd3, efd4;

void *writer_routine(void *arg)
{
        cpu_set_t mask;
        uint64_t val;
        int ret;

        CPU_ZERO(&mask);
        CPU_SET((unsigned long)arg % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        /* EFD_SEMAPHORE */
        for (int i = 0; i < WRITECNT; i++) {
                val = WRITEVAL;
                ret = write(efd1, &val, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t), "write failed!");
        }

        /* EFD_NONBLOCK */
        for (int i = 0; i < WRITECNT; i++) {
                val = WRITEVAL;
                ret = write(efd2, &val, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t), "write failed!");
        }

        /* Larger than max */
        /* BLOCK WRITER */
        val = UINT64_MAX;
        ret = write(efd3, &val, sizeof(uint64_t));
        chcore_assert(ret == -1 && errno == EINVAL,
                "write status check failed!");

        val = UINT64_MAX - 1;
        ret = write(efd3, &val, sizeof(uint64_t));
        chcore_assert(ret == sizeof(uint64_t), "write failed!");

        /* EFD_SEMAPHORE BLOCK WRITER */
        for (int i = 0; i < WRITECNT; i++) {
                val = WRITEVAL;
                ret = write(efd4, &val, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t), "write failed!");
        }

        info("Writer thread %lu finished!\n", (unsigned long)arg);
        return 0;
}

void *reader_routine(void *arg)
{
        cpu_set_t mask;
        uint64_t val, sum = 0;
        int ret;

        /* EFD_SEMAPHORE BLOCK READER */
        CPU_ZERO(&mask);
        CPU_SET((unsigned long)arg % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        for (int i = 0; i < WRITECNT * WRITEVAL; i++) {
                ret = read(efd1, &val, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t), "read failed!");
                chcore_assert(val == 1, "EFD_SEMAPHORE value check failed!");
        }

        /* EFD_NONBLOCK */
        for (int i = 0; i < WRITECNT * WRITEVAL; i++) {
                ret = read(efd2, &val, sizeof(uint64_t));
                sum += (ret == sizeof(uint64_t)) ? val : 0;
        }

        /* BLOCK WRITER */
        ret = read(efd3, &val, sizeof(uint64_t));
        chcore_assert(ret == sizeof(uint64_t), "read failed!");
        chcore_assert(val == UINT64_MAX - 1, "read value check failed!");

        /* EFD_SEMAPHORE BLOCK WRITER */
        for (int i = 0; i < WRITECNT * WRITEVAL; i++) {
                ret = read(efd4, &val, sizeof(uint64_t));
                chcore_assert(ret == sizeof(uint64_t), "read failed!");
                chcore_assert(val == 1, "EFD_SEMAPHORE value check failed!");
        }

        info("Reader thread %lu finished\n", (unsigned long)arg);
        return (void *)(unsigned long)sum;
}

#define INIT_VAL 10

int main(int argc, char *argv[])
{
        pthread_t writer[THREAD_NUM], reader[THREAD_NUM];
        uint64_t efd2_sum = 0, ret, val;
        void *thread_ret;

        info("Test eventfd begins...\n");

        efd1 = eventfd(0, EFD_SEMAPHORE);
        chcore_assert(efd1 >= 0, "create eventfd failed!");
        efd2 = eventfd(INIT_VAL, EFD_NONBLOCK);
        chcore_assert(efd2 >= 0, "create eventfd failed!");
        efd3 = eventfd(0, 0);
        chcore_assert(efd3 >= 0, "create eventfd failed!");
        efd4 = eventfd(0, EFD_SEMAPHORE);
        chcore_assert(efd4 >= 0, "create eventfd failed!");

        val = UINT64_MAX - 1;
        ret = write(efd4, &val, sizeof(uint64_t));
        chcore_assert(ret == sizeof(uint64_t), "write failed!");

        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_create(&reader[i],
                               NULL,
                               reader_routine,
                               (void *)(unsigned long)i);
                pthread_create(&writer[i],
                               NULL,
                               writer_routine,
                               (void *)(unsigned long)i);
        }

        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_join(reader[i], (void **)&thread_ret);
                pthread_join(writer[i], NULL);
                efd2_sum += (unsigned long)thread_ret;
        }
        ret = read(efd2, &val, sizeof(uint64_t));
        efd2_sum += (ret == sizeof(uint64_t)) ? val : 0;
        chcore_assert(efd2_sum == WRITECNT * WRITEVAL * THREAD_NUM + INIT_VAL,
                      "EFD_NONBLOCK value check failed!");

        green_info("Test eventfd finished!\n");

        return 0;
}
