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

#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/shm.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <sched.h>

#include "test_tools.h"

#define SHM_NUM 1

int main()
{
        int shmids[SHM_NUM];
        void *volatile shmaddrs[SHM_NUM];
        unsigned long volatile magic = 0;
        int i;
        unsigned long magic1 = 0x56789;
        unsigned long magic2 = 0x12345;
        unsigned long server_read_flag = 0x132;

        printf("test_shm_server starts\n");

        /* Get, map and read some shms */
        for (i = 0; i < SHM_NUM; ++i) {
                printf("start server shmget\n");
                shmids[i] = shmget(i + 1, 0x1000, IPC_CREAT);
                if (shmids[i] <= 0) {
                        printf("[Error] shmget fail, shmget return 0, errno:%d\n",
                               errno);
                        return -1;
                }
                printf("[server]: shmget success: %d\n", shmids[i]);
                shmaddrs[i] = (void *)shmat(shmids[i], 0, 0);
                if (shmaddrs[i] == (void *)-1) {
                        printf("[Error] shmat fail, shmat return (void *) -1, errno:%d\n",
                               errno);
                        return -1;
                }
                printf("[server]: shmat success: %p\n", shmaddrs[i]);
                while (magic != magic1) {
                        magic = *(unsigned long *)shmaddrs[i];
                        sched_yield();
                }
                *(unsigned long *)shmaddrs[i] = server_read_flag;
        }

        /* Unmap these shms */
        for (i = 0; i < SHM_NUM; ++i) {
                if (shmdt(shmaddrs[i]) < 0) {
                        printf("[Error] shmdt fail, shmdt return -1, errno:%d\n",
                               errno);
                        printf("[server]: error addr: %p\n", shmaddrs[i]);
                        return -1;
                }
        }

        /*
         * Get, map and read these shms again.
         * Print magic numbers read from shm.
         */
        for (i = 0; i < SHM_NUM; ++i) {
                shmids[i] = shmget(i + 1, 0x1000, IPC_CREAT);
                if (shmids[i] <= 0) {
                        printf("[Error] shmget fail, shmget return 0, errno:%d\n",
                               errno);
                        return -1;
                }
                printf("[server]: shmget success: %d\n", shmids[i]);
                shmaddrs[i] = (void *)shmat(shmids[i], 0, 0);
                if (shmaddrs[i] == (void *)-1) {
                        printf("[Error] shmat fail, shmat return (void *) -1, errno:%d\n",
                               errno);
                        return -1;
                }
                printf("[server]: shmat success: %p\n", shmaddrs[i]);
                while (magic != magic2) {
                        magic = *(unsigned long *)shmaddrs[i];
                        sched_yield();
                }
                printf("shmid=%x, shmaddr=%lx, magic=%lx\n",
                       shmids[i],
                       (unsigned long)shmaddrs[i],
                       magic);
                if (shmdt(shmaddrs[i]) < 0) {
                        printf("[Error] shmdt faile, shmdt return -1, errno:%d\n",
                               errno);
                        printf("[server]: error addr: %p\n", shmaddrs[i]);
                        return -1;
                }
                if (shmctl(shmids[i], IPC_RMID, NULL) < 0) {
                        printf("[Error] shmctl fail, shmctl return -1, errno:%d\n",
                               errno);
                        return -1;
                }
        }

        printf("test_shm_server finished\n");
        return 0;
}
