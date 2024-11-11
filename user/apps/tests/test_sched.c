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
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_tools.h"

#define TOTAL_TEST_NUM 16
#define SCHED_TEST_NUM 32

/* There is not so much VM space on 32-bit machine. */
#if __SIZEOF_POINTER__ == 4
#define THREAD_NUM 128
#else
#define THREAD_NUM 512
#endif

/*
 * We do not use semaphore because this is an independent test
 * and we cannot assume semaphore is correctly implemented.
 * Semaphore is tested in `test_notific.c`.
 */
volatile int finish_flag;

void sched_test(int cpuid)
{
        cpu_set_t mask;
        int ret = 0;

        /* pthread not implemented, should add some other threads */
        for (int i = 0; i < SCHED_TEST_NUM; i++) {
                CPU_ZERO(&mask);
                CPU_SET(i % PLAT_CPU_NUM, &mask);
                ret = sched_setaffinity(0, sizeof(mask), &mask);
                chcore_assert(ret == 0, "sched_setaffinity failed!");
                sched_yield();
                CPU_ZERO(&mask);
                ret = sched_getaffinity(0, sizeof(mask), &mask);
                chcore_assert(ret == 0, "sched_getaffinity failed!");
                for (int j = 0; j < 128; j++) {
                        if (CPU_ISSET(j, &mask)) {
                                ret = j;
                                break;
                        }
                }
                chcore_assert((i % PLAT_CPU_NUM) == ret,
                              "sched_setaffinity check failed!");
        }

        /* set a fault cpuid */
        CPU_ZERO(&mask);
        CPU_SET(cpuid + 128, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret != 0, "sched_setaffinity fault check failed!");

        /* set to cpuid */
        CPU_ZERO(&mask);
        CPU_SET(cpuid, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();
}

void *thread_routine(void *arg)
{
        unsigned long tid = (unsigned long)arg;

        for (int i = 0; i < TOTAL_TEST_NUM; i++) {
                sched_test((tid + i) % PLAT_CPU_NUM);
        }
        __atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);
        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int thread_num = THREAD_NUM;
        pthread_t thread_id[THREAD_NUM];

        info("Test sched begins...\n");

        if (argc >= 2) {
                thread_num = atoi(argv[1]);
                if (thread_num > THREAD_NUM) {
                        thread_num = THREAD_NUM;
                }
        }
        info("Test thread number is %d\n", thread_num);

        finish_flag = 0;
        for (int i = 0; i < thread_num; i++) {
                pthread_create(&thread_id[i],
                               NULL,
                               thread_routine,
                               (void *)(unsigned long)i);
        }
        while (finish_flag != THREAD_NUM) {
                sched_yield();
        }

        for (int i = 0; i < thread_num; i++) {
                pthread_join(thread_id[i], NULL);
        }

        green_info("Test sched finished!\n");

        return 0;
}
