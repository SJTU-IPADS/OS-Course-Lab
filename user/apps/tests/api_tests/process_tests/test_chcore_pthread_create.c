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

#include <chcore/pthread.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

void *new_thread(void *arg)
{
        printf("New thread is running!\n");
        return 0;
}

int main()
{
        pthread_t tid;
        chcore_pthread_create(&tid, NULL, new_thread, NULL);
        assert(tid > 0);
        printf("chcore_pthread_create success.\n");
        pthread_join(tid, NULL);
        printf("Test finish!\n");
        return 0;
}
