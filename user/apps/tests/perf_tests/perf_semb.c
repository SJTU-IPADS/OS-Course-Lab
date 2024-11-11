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
#include <semLib.h>
#include <stdio.h>
#include <stdlib.h>

#include "perf_tools.h"

#define PERF_NUM                1000

pthread_barrier_t barrier;
SEM_ID sem;
unsigned volatile waked;

void *thread_routine_side_a(void *arg)
{
        int i;

        if (bind_thread_to_core(2) < 0)
                error("side a sched_setaffinity failed!\n");

        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));
        pthread_barrier_wait(&barrier);

        record_time(start_perf_time);
        for (i = 0; i < PERF_NUM; i++) {
                semTake(sem, WAIT_FOREVER);
        }
        record_time(end_perf_time);
        waked = true;

        printf("Perf Semaphore Result:\n");
        print_perf_result(start_perf_time, end_perf_time, PERF_NUM);

        return NULL;
}

void *thread_routine_side_b(void *arg)
{
        if (bind_thread_to_core(3) < 0)
                error("side b sched_setaffinity failed!\n");

        pthread_barrier_wait(&barrier);
        while (!waked) {
                semGive(sem);
                sched_yield();
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        int i;
        pthread_t tid[2];

        pthread_barrier_init(&barrier, NULL, 2);
        sem = semBCreate(0, SEM_EMPTY);
        waked = false;

        pthread_create(&tid[0], NULL, thread_routine_side_a, NULL);
        pthread_create(&tid[1], NULL, thread_routine_side_b, NULL);

        for (i = 0; i < 2; i++) {
                pthread_join(tid[i], NULL);
        }

        semDelete(sem);
        return 0;
}
