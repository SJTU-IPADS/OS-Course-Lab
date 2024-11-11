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

#include "test_tools.h"

#define CLIENT_NUM 1
#define SERVER_NUM 1

/*
 * Create several servers and clients to test shm functions.
 * If succ, ALL test_shm_server will exit successfully.
 */
int main()
{
        char *args_client[1];
        char *args_server[1];
        struct new_process_caps client_process_caps[CLIENT_NUM];
        struct new_process_caps server_process_caps[SERVER_NUM];
        int i;

        args_client[0] = "/test_shm_client.bin";
        args_server[0] = "/test_shm_server.bin";

        /* Create some server processes */
        for (i = 0; i < SERVER_NUM; ++i) {
                create_process(1, args_server, &server_process_caps[i]);
        }

        /* Create some client processes */
        for (i = 0; i < CLIENT_NUM; ++i) {
                create_process(1, args_client, &client_process_caps[i]);
        }

        printf("test_shm_multi finished\n");
        return 0;
}
