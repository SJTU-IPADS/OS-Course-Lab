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

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "test_tools.h"

#define THREAD_NUM 32
#define TEST_NUM   50000

int g_varible;
pthread_mutex_t g_lock;

void *thread_routine(void *var)
{
        int tid = (int)(unsigned long)var;
#ifdef BIND
        int ret = 0;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(tid % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "Set affinity failed!");
#endif

        info("Pthread %d alive!\n", tid);
        for (int i = 0; i < TEST_NUM; i++) {
                pthread_mutex_lock(&g_lock);
                g_varible++;
                pthread_mutex_unlock(&g_lock);
        }
        return var;
}

int main(int argc, char *argv[])
{
        pthread_t thread_id[THREAD_NUM];
        void *ret = 0;

        info("Test pthread begins...\n");

        pthread_mutex_init(&g_lock, NULL);
        g_varible = 0;

        for (int i = 0; i < THREAD_NUM; i++)
                pthread_create(&thread_id[i],
                               NULL,
                               thread_routine,
                               (void *)(unsigned long)i);
        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_join(thread_id[i], &(ret));
                chcore_assert(((unsigned long)ret == (unsigned long)i),
                              "return value check failed!");
        }

        chcore_assert(g_varible == TEST_NUM * THREAD_NUM,
                      "pthread lock failed!");

        green_info("Test pthread finished!\n");
        return 0;
}
