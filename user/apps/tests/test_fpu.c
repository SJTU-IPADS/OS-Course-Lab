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

#define FPU_TEST_NUM (256)
#define THREAD_NUM   32

int global_error = 0;
volatile int finish_flag;

void test_fpu(int base)
{
        double *fnums;

        fnums = malloc(FPU_TEST_NUM * sizeof(double));

        for (int i = 0; i < FPU_TEST_NUM; i++) {
                fnums[i] = base + 1.0 / (2 + i);
        }

        for (int i = 0; i < 1000000; i++) {
                for (int j = 0; j < FPU_TEST_NUM; ++j) {
                        chcore_assert((fnums[j] > base) && (fnums[j] < base + 1),
                                "wrong float value!");
                }
        }

        free(fnums);
}

void *thread_routine(void *arg)
{
        int tid = (int)(long)arg, ret = 0;

        /* binding to different cores */
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(tid % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        test_fpu(tid + 1);

        __atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);

        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        int thread_num = THREAD_NUM;
        pthread_t thread_id[THREAD_NUM];

        info("Test FPU begins...\n");

        if (argc >= 2) {
                thread_num = atoi(argv[1]);
                if (thread_num > THREAD_NUM) {
                        thread_num = THREAD_NUM;
                }
        }
        info("Test thread number is %d\n", thread_num);

        global_error = 0;
        finish_flag = 0;

        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_create(&thread_id[i],
                               NULL,
                               thread_routine,
                               (void *)(unsigned long)i);
        }
        while (finish_flag != THREAD_NUM) {
                sched_yield();
        }

        chcore_assert(global_error == 0, "error occurs: %d", global_error);

        green_info("Test FPU finished!\n");

        return 0;
}
