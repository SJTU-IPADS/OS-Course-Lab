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
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include "perf_tools.h"

#define PERF_TEST_NUM PERF_NUM_SMALL

volatile cap_t notific_cap;
perf_time_t *start_perf_time;
perf_time_t *end_perf_time;
perf_time_t *total_perf_time;
perf_time_t *start_perf_time2;
perf_time_t *end_perf_time2;
perf_time_t *total_perf_time2;
volatile int ready = 0;

void *wait_thread_routine(void *arg)
{
        if (bind_thread_to_core(2) < 0)
                error("sched_setaffinity failed!\n");

        record_time(start_perf_time);
        usys_wait(notific_cap, 0, 0);
        record_time(end_perf_time);
        accumulate_perf_record(start_perf_time, end_perf_time, total_perf_time);
        return 0;
}

void *notify_thread_routine(void *arg)
{
        if (bind_thread_to_core(2) < 0)
                error("sched_setaffinity failed!\n");

        record_time(start_perf_time2);
        usys_notify(notific_cap);
        record_time(end_perf_time2);
        accumulate_perf_record(
                start_perf_time2, end_perf_time2, total_perf_time2);
        return 0;
}

void *block_thread_routine(void *arg)
{
        if (bind_thread_to_core(3) < 0)
                error("sched_setaffinity failed!\n");

        ready = 1;
        usys_wait(notific_cap, 1, 0);
        record_time(end_perf_time);
        accumulate_perf_record(start_perf_time, end_perf_time, total_perf_time);
        ready = 0;
        return 0;
}

int main()
{
        if (bind_thread_to_core(3) < 0)
                error("sched_setaffinity failed!\n");

        start_perf_time = malloc(sizeof(perf_time_t));
        end_perf_time = malloc(sizeof(perf_time_t));
        total_perf_time = malloc(sizeof(perf_time_t));
        start_perf_time2 = malloc(sizeof(perf_time_t));
        end_perf_time2 = malloc(sizeof(perf_time_t));
        total_perf_time2 = malloc(sizeof(perf_time_t));
        total_perf_time->time_sum = 0;
        total_perf_time->cycle_sum = 0;
        total_perf_time2->time_sum = 0;
        total_perf_time2->cycle_sum = 0;

        pthread_t thread_id[3];
        int i = 0;
        notific_cap = usys_create_notifc();

        for (i = 0; i < PERF_TEST_NUM; i++) {
                pthread_create(&thread_id[2], NULL, block_thread_routine, NULL);
                while (!ready)
                        asm volatile("nop" ::: "memory");
                record_time(start_perf_time);
                usys_notify(notific_cap);
                sched_yield();
                pthread_join(thread_id[2], NULL);
        }

        info("thread wake up (including calling sys_notific) result:\n");
        print_accumulate_perf_result(total_perf_time, PERF_TEST_NUM);

        for (i = 0; i < PERF_TEST_NUM; i++) {
                pthread_create(&thread_id[0], NULL, wait_thread_routine, NULL);
                pthread_create(
                        &thread_id[1], NULL, notify_thread_routine, NULL);
                pthread_join(thread_id[0], NULL);
                pthread_join(thread_id[1], NULL);
        }

        info("call sys_wait result:\n");
        print_accumulate_perf_result(total_perf_time, PERF_TEST_NUM);
        info("call sys_notific result:\n");
        print_accumulate_perf_result(total_perf_time2, PERF_TEST_NUM);

        free(start_perf_time);
        free(end_perf_time);
        free(total_perf_time);
        free(start_perf_time2);
        free(end_perf_time2);
        free(total_perf_time2);

        return 0;
}