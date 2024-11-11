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
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <sched.h>

#include "test_tools.h"

/* Testing mmap */
#define TEST_MMAP 0
/* Testing malloc */
#define TEST_MALLOC 1

#define TEST_OPTION TEST_MALLOC

#define MALLOC_TEST_NUM (256 * 2)
#define THREAD_NUM      16

unsigned long buffer_array[MALLOC_TEST_NUM * THREAD_NUM];

static void check_overlap(void)
{
        for (int i = 0; i < MALLOC_TEST_NUM * THREAD_NUM; ++i) {
                for (int j = i + 1; j < MALLOC_TEST_NUM * THREAD_NUM; ++j) {
                        if ((buffer_array[i] != 0) && (buffer_array[j] != 0)) {
                                chcore_assert(
                                    labs(buffer_array[i] - buffer_array[j])
                                    >= 0x1000, "memory overlapped: "
                                    "%d: 0x%lx vs. %d: 0x%lx",
                                    i, buffer_array[i], j, buffer_array[j]);
                        }
                }
        }
        info("No overlapped memory!\n");
}

void malloc_test(int tid)
{
        int size = 0;
        char *buf[MALLOC_TEST_NUM];
        char target_val = tid + 9;

        for (int i = 0; i < MALLOC_TEST_NUM; ++i) {
                size = 0x1000;
                buf[i] = NULL;
#if TEST_OPTION == TEST_MALLOC
                buf[i] = malloc(size);
#elif TEST_OPTION == TEST_MMAP
                buf[i] = (char *)mmap(0,
                                      size,
                                      PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS,
                                      -1,
                                      0);
#endif
                buffer_array[tid * MALLOC_TEST_NUM + i] = (long)buf[i];
                chcore_assert(buf[i] != NULL, "malloc failed!");

                for (int j = 0; j < size; ++j) {
                        *((char *)buf[i] + j) = target_val;
                        chcore_assert(*((char *)buf[i] + j) == target_val,
                                "write memory failed!");
                }
        }

        /* Check consistency again. */
        for (int i = 0; i < MALLOC_TEST_NUM; ++i) {
                size = 0x1000;
                for (int j = 0; j < size; ++j) {
                        chcore_assert(*((char *)buf[i] + j) == target_val,
                                "write memory failed!");
                }
        }

        info("Thread %d finished!\n", tid);
}

volatile int finish_flag;

void *thread_routine(void *arg)
{
        int tid = (int)(long)arg, ret = 0;

        /* Bind to different cores. */
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(tid % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        malloc_test(tid);

        __atomic_add_fetch(&finish_flag, 1, __ATOMIC_SEQ_CST);

        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        pthread_t thread_id;

        finish_flag = 0;

        info("Test malloc begins...\n");

        for (int i = 0; i < THREAD_NUM; i++)
                pthread_create(&thread_id,
                               NULL,
                               thread_routine,
                               (void *)(unsigned long)i);

        while (finish_flag != THREAD_NUM) {
                usys_yield();
        }

        check_overlap();

        green_info("Test malloc finished!\n");

        return 0;
}
