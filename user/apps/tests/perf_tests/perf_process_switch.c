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

#define PRIO 255

/* Task switch benchmark */
#define TASKSWITCH_COUNT PERF_NUM_LARGE

/*
 * Use Shared Memory For lock and barrier
 * shmget(), shmat(), shmdt(), shmctl(), pthread_spin_lock()
 * Use create_process() for second process
 * Use sched_yield() to sched to another thread
 * Use setaffinity to keep the processes on core3
 */

int main()
{
        if (bind_thread_to_core(3) < 0)
                error("sched_setaffinity failed!\n");

        cap_t shm_cap;
        int shm_cap2;
        int *addr;
        pthread_spinlock_t *barrier_lock;

        shm_cap = shmget(1, 0x1000, IPC_CREAT);
        shm_cap2 = shmget(2, 0x1000, IPC_CREAT);
        barrier_lock = (pthread_spinlock_t *)shmat(shm_cap, 0, 0);
        addr = shmat(shm_cap2, 0, 0);

        addr[0] = 2;
        pthread_spin_init(barrier_lock, PTHREAD_PROCESS_PRIVATE);

        char *args[1];
        args[0] = "/perf_second_process.bin";
        create_process(1, args, NULL);
        printf("Start measure process switch\n");

        int j = TASKSWITCH_COUNT;
        perf_time_t *start_perf_time = malloc(sizeof(perf_time_t));
        perf_time_t *end_perf_time = malloc(sizeof(perf_time_t));
        volatile int finish = 0;

        pthread_spin_lock(barrier_lock);
        addr[0]--;
        pthread_spin_unlock(barrier_lock);

        while (addr[0] != 0)
                asm volatile("nop" ::: "memory");

        record_time(start_perf_time);

        while (j--) {
                sched_yield();
        }

        pthread_spin_lock(barrier_lock);
        addr[0]++;
        pthread_spin_unlock(barrier_lock);
        asm volatile("nop" ::: "memory");

        record_time(end_perf_time);

        info("Process switch result:\n");
        print_perf_result(start_perf_time, end_perf_time, TASKSWITCH_COUNT * 2);
        finish = 1;

        while (finish == 0)
                ;
        info("process switch exited\n");
        shmdt(addr);
        shmdt(barrier_lock);
        shmctl(shm_cap, IPC_RMID, NULL);
        shmctl(shm_cap2, IPC_RMID, NULL);

        free(start_perf_time);
        free(end_perf_time);

        return 0;
}
