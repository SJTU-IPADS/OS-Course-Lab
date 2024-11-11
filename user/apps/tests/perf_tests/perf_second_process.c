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
#include <chcore/launcher.h>
#include <sys/shm.h>

#include "perf_tools.h"

/* Task switch benchmark */
#define TASKSWITCH_COUNT PERF_NUM_LARGE

int main()
{
        if (bind_thread_to_core(3) < 0)
                error("sched_setaffinity failed!\n");

        int j = TASKSWITCH_COUNT;
        cap_t shm_cap, shm_cap2;
        int *addr;
        pthread_spinlock_t *barrier_lock;
        shm_cap = shmget(1, 0x1000, IPC_CREAT);
        shm_cap2 = shmget(2, 0x1000, IPC_CREAT);
        barrier_lock = (pthread_spinlock_t *)shmat(shm_cap, 0, 0);
        addr = shmat(shm_cap2, 0, 0);

        pthread_spin_lock(barrier_lock);
        addr[0]--;
        pthread_spin_unlock(barrier_lock);
        sched_yield();

        while (addr[0] != 0)
                asm volatile("nop" ::: "memory");

        while (j--) {
                sched_yield();
        }

        pthread_spin_lock(barrier_lock);
        addr[0]++;
        pthread_spin_unlock(barrier_lock);
        asm volatile("nop" ::: "memory");

        shmdt(addr);
        shmdt(barrier_lock);
        printf("process 2 exited\n");

        return 0;
}