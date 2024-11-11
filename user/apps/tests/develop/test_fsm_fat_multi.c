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
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <sys/mount.h>

#include "test_tools.h"

int main()
{
        char *args_client[3];
        struct new_process_caps client_process_caps[3];
        int i;

        mount("", "/home", 0, 0, 0);
        args_client[0] = "/test_fsm_fat_child1.bin";
        args_client[1] = "/test_fsm_fat_child2.bin";
        args_client[2] = "/test_fsm_fat_child3.bin";
        /* Create some client processes */
        for (i = 0; i < 3; ++i) {
                create_process(1, &args_client[i], &client_process_caps[i]);
        }

        while (1) {
                sched_yield();
        }

        printf("test_fsm_fat_multi finished\n");
        return 0;
}