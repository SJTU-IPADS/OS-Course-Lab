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
#define __USE_GNU // For coding on x86_64 Linux
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <chcore/syscall.h>

#include "perf_tools.h"

#define PERF_TEST_NUM PERF_NUM_LARGE

sem_t* sem0;
sem_t* sem1;
u8 finished = 0;
u8 writer_ok = 0;
perf_time_t* start_perf_time;
perf_time_t* end_perf_time;

void* doit(void* args)
{
        bind_thread_to_core(3);
        record_time(start_perf_time);
        for (int i = 0; i < PERF_TEST_NUM; i++) {
                sem_wait(sem1);
                sem_post(sem0);
        }
        record_time(end_perf_time);
        return 0;
}

void* writer(void* args)
{
        bind_thread_to_core(3);
        writer_ok = 1;
        sem_post(sem1);
        while (!finished) {
                sem_wait(sem0);
                sem_post(sem1);
        }
        return NULL;
}

int main()
{
        sem0 = (sem_t*)malloc(sizeof(sem_t));
        sem1 = (sem_t*)malloc(sizeof(sem_t));
        sem_init(sem0, 0, 0);
        sem_init(sem1, 0, 0);

        start_perf_time = malloc(sizeof(perf_time_t));
        end_perf_time = malloc(sizeof(perf_time_t));

        pthread_t doit_thread;
        pthread_t writer_thread;

        pthread_create(&writer_thread, NULL, writer, NULL);
        while (!writer_ok) {
                asm volatile("nop" ::: "memory");
        }
        pthread_create(&doit_thread, NULL, doit, NULL);
        pthread_join(doit_thread, NULL);

        finished = 1;
        sem_post(sem0);
        pthread_join(writer_thread, NULL);

        print_perf_result(start_perf_time, end_perf_time, PERF_TEST_NUM * 2);
        free(sem0);
        free(sem1);
        free(start_perf_time);
        free(end_perf_time);
}
