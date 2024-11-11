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
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <chcore/syscall.h>

pthread_mutex_t lock;

void *high_prio_thread(void *var)
{
        cpu_set_t mask;
        int ret;

        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        assert(ret == 0);
        usys_set_prio(0, 10);
        usys_yield();

        sleep(5);

        printf("high before lock\n");
        pthread_mutex_lock(&lock);
        printf("high locked\n");

        return NULL;
}

void *low_prio_thread(void *var)
{
        cpu_set_t mask;
        int ret;
        struct timespec t0, t1;

        CPU_ZERO(&mask);
        CPU_SET(0, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        assert(ret == 0);
        usys_set_prio(0, 5);
        usys_yield();

        printf("low before lock\n");
        pthread_mutex_lock(&lock);
        printf("low locked\n");

        clock_gettime(0, &t0);
        do {
                clock_gettime(0, &t1);
        } while (t1.tv_sec - t0.tv_sec < 10);

        printf("low before unlock\n");
        pthread_mutex_unlock(&lock);

        return NULL;
}

int main()
{
        pthread_t high_prio_t, low_prio_t;

        pthread_mutex_init(&lock, NULL);
        pthread_mutex_setprioceiling(&lock, 11, NULL);

        pthread_create(&high_prio_t, NULL, high_prio_thread, NULL);
        pthread_create(&low_prio_t, NULL, low_prio_thread, NULL);

        pthread_join(high_prio_t, NULL);
        pthread_join(low_prio_t, NULL);

        return 0;
}
