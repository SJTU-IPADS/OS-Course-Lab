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

#include <chcore/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <time.h>

#include "test_tools.h"

#define SLEEP_COUNT     3000
#define THREAD_NUM      16  /* Per core */

static cap_t notifc_cap;
static pthread_barrier_t test_end_barrier, test_start_barrier;

void *sleeper(void *arg)
{
        struct timespec ts;
        int ret, thread_id = (int)(long)arg;

        cpu_set_t mask;

        /* Notification timeout test. */
        pthread_barrier_wait(&test_start_barrier);
        for (int i = 0; i < SLEEP_COUNT; i++) {
                CPU_ZERO(&mask);
                CPU_SET((thread_id + i) % PLAT_CPU_NUM, &mask);
                ret = sched_setaffinity(0, sizeof(mask), &mask);
                chcore_assert(ret == 0, "sched_setaffinity failed!");

                if (thread_id % 2) {
                        /* Wait with timeout of 10 ms. */
                        ts.tv_sec = 0;
                        ts.tv_nsec = 10 * 1000 * 1000;
                        ret = usys_wait(notifc_cap, true, &ts);
                        chcore_assert(ret == 0 || ret == -ETIMEDOUT,
                                "usys_wait failed!");
                } else {
                        /* Signal every 15 ms. */
                        ts.tv_sec = 0;
                        ts.tv_nsec = 15 * 1000 * 1000;
                        nanosleep(&ts, NULL);
                        ret = usys_notify(notifc_cap);
                        chcore_assert(ret == 0, "usys_notify failed!");
                }
        }
        pthread_barrier_wait(&test_end_barrier);

        return NULL;
}

int main()
{
        pthread_t pthreads[THREAD_NUM * PLAT_CPU_NUM];
        int tid;

        info("Test sleep begins...\n");

        notifc_cap = usys_create_notifc();
        chcore_assert(notifc_cap >= 0, "create notific failed!");

        pthread_barrier_init(
                &test_end_barrier, NULL, THREAD_NUM * PLAT_CPU_NUM + 1);
        pthread_barrier_init(
                &test_start_barrier, NULL, THREAD_NUM * PLAT_CPU_NUM + 1);

        for (int i = 0; i < THREAD_NUM; i++) {
                for (int j = 0; j < PLAT_CPU_NUM; j++) {
                        tid = i * PLAT_CPU_NUM + j;
                        pthread_create(&pthreads[tid],
                                       NULL,
                                       sleeper,
                                       (void *)(long)tid);
                }
        }

        pthread_barrier_wait(&test_start_barrier);
        pthread_barrier_wait(&test_end_barrier);

        for (int i = 0; i < THREAD_NUM * PLAT_CPU_NUM; i++) {
                pthread_join(pthreads[i], NULL);
        }

        green_info("Test sleep finished!\n");

        return 0;
}
