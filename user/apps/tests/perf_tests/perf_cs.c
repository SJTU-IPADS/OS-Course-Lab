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
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <chcore/syscall.h>

#include "perf_tools.h"

/* Task switch benchmark */
#define TASKSWITCH_COUNT PERF_NUM_LARGE

volatile int barrier = 0;
pthread_spinlock_t barrier_lock;

void *switch_bench(void *arg)
{
        if (bind_thread_to_core(3) < 0)
                info("No core binding!\n");

        int i = TASKSWITCH_COUNT;
        int j = (int)(long)arg;

        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));

        pthread_spin_lock(&barrier_lock);
        barrier--;
        pthread_spin_unlock(&barrier_lock);
        sched_yield();

        while (barrier != 0)
                ;
        if (j != 0) {
                record_time(start_perf_time);
        }
        while (i--)
                sched_yield();

        if (j != 0) {
                record_time(end_perf_time);
                asm volatile("nop" ::: "memory");
                info("Content switch Result:\n");
                print_perf_result(
                        start_perf_time, end_perf_time, TASKSWITCH_COUNT * 2);
        }

        free(start_perf_time);
        free(end_perf_time);

        return 0;
}

int main(int argc, char *argv[])
{
        int ret;
        pthread_t threads[2];
        pthread_attr_t pthread_attr;
        struct sched_param sched_param;
        int i;
        
        info("Start measure context switch\n");

        usys_set_prio(0, 100);
        pthread_spin_init(&barrier_lock, PTHREAD_PROCESS_PRIVATE);
        barrier = 2;
        sched_param.sched_priority = 50;
        pthread_attr_init(&pthread_attr);
        pthread_attr_setinheritsched(&pthread_attr, 1);
        pthread_attr_setschedparam(&pthread_attr, &sched_param);

        for (i = 0; i < 2; i++) {
                ret = pthread_create(&threads[i], &pthread_attr, switch_bench, (void *)(unsigned long)i);
                if (ret < 0)
                        info("Create thread failed, return %d\n", ret);
        }
        usys_set_prio(0, 1);
        for (i = 0; i < 2; i++) {
                pthread_join(threads[i], NULL);
        }

        return 0;
}
