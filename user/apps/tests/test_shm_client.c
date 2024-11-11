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

#include "test_tools.h"

#define SHM_NUM    1

/*
 * Execute several shm operations on several shms.
 * Using shmget, shmat, shmdt.
 */
void *client_routine(void *arg)
{
        int shmid[SHM_NUM];
        void *shmaddr[SHM_NUM];
        unsigned long volatile magic = 0;
        unsigned long magic1 = 0x56789;
        unsigned long magic2 = 0x12345;
        unsigned long server_read_flag = 0x132;
        int i;

        for (i = 0; i < SHM_NUM; ++i) {
                shmid[i] = shmget(i + 1, 0x1000, IPC_CREAT);
                if (shmid[i] <= 0) {
                        printf("[Error] shmget fail, shmget return 0, errno:%d\n",
                               errno);
                        return (void *)-1;
                }
                printf("[client]: shmget success: %d\n", shmid[i]);
                shmaddr[i] = (void *)shmat(shmid[i], 0, 0);
                if (shmaddr[i] == (void *)-1) {
                        printf("[Error] shmat fail, shmat return (void *) -1, errno:%d\n",
                               errno);
                        return (void *)-1;
                }
                printf("[client]: shmat success: %p\n", shmaddr[i]);
                *(unsigned long *)shmaddr[i] = magic1;
                while (magic != server_read_flag) {
                        magic = *(unsigned long *)shmaddr[i];
                        sched_yield();
                }
        }

        for (i = 0; i < SHM_NUM; ++i) {
                if (shmdt(shmaddr[i]) < 0) {
                        printf("[Error] shmdt fail, shmdt return -1, errno:%d\n",
                               errno);
                        printf("[client]: error addr: %p\n", shmaddr[i]);
                        return (void *)-1;
                }
        }

        for (i = 0; i < SHM_NUM; ++i) {
                shmid[i] = shmget(i + 1, 0x1000, IPC_CREAT);
                if (shmid[i] <= 0) {
                        printf("[Error] shmget fail, shmget return 0, errno:%d\n",
                               errno);
                        return (void *)-1;
                }
                printf("[client]: shmget success: %d\n", shmid[i]);
                shmaddr[i] = (void *)shmat(shmid[i], 0, 0);
                if (shmaddr[i] == (void *)-1) {
                        printf("[Error] shmat fail, shmat return (void *) -1, errno:%d\n",
                               errno);
                        return (void *)-1;
                }
                printf("[client]: shmat success: %p\n", shmaddr[i]);
                *(unsigned long *)shmaddr[i] = magic2;
        }

        for (i = 0; i < SHM_NUM; ++i) {
                if (shmdt(shmaddr[i]) < 0) {
                        printf("[Error] shmdt faile, shmdt return -1, errno:%d\n",
                               errno);
                        printf("[client]: error addr: %p\n", shmaddr[i]);
                        return (void *)-1;
                }
        }

        return 0;
}

int main()
{
        pthread_t thread_id;
        void *ret = 0;

        printf("test_shm_client starts\n");

        /* Create a pthread to do client_routine */
        pthread_create(&thread_id,
                        NULL,
                        client_routine,
                        NULL);
        pthread_join(thread_id, &(ret));
        chcore_assert((unsigned long)ret == 0,
                        "Return value check failed!");

        printf("test_shm_client finished\n");
        return 0;
}
