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

#include <pthread.h>
#include <stdio.h>

#define THREAD_NUM 10

void* fault_routine(void* p)
{
        printf("%s\n", (char*)1);
        return NULL;
}

int main()
{
        pthread_t threads[THREAD_NUM];

        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_create(&threads[i], NULL, fault_routine, NULL);
        }

        for (int i = 0; i < THREAD_NUM; i++) {
                pthread_join(threads[i], NULL);
        }

        return 0;
}