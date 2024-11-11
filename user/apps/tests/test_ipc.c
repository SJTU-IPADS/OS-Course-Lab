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

#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <chcore/type.h>
#include <chcore/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_tools.h"

#define IPC_NUM 1000

/*
 * Due to the limited VM space on 32-bit machines,
 * we just test less ipc clients on it.
 */
#if __SIZEOF_POINTER__ == 4
#define TEST_IPC_CLIENT_NUM 2
#else
#define TEST_IPC_CLIENT_NUM 20
#endif

cap_t server_cap;

struct server_exit_msg {
        int server_exit_flag;
};

void *perf_ipc_routine(void *arg)
{
        ipc_struct_t *client_ipc_struct;
        cpu_set_t mask;
        int tid = (int)(long)arg, ret;
        ipc_msg_t *ipc_msg;
        struct server_exit_msg *se;

        CPU_ZERO(&mask);
        CPU_SET(tid % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        /* register mutiple IPC clients */
        for (int i = 0; i < TEST_IPC_CLIENT_NUM; ++i) {
                client_ipc_struct = ipc_register_client(server_cap);
                chcore_assert(client_ipc_struct != NULL,
                        "ipc_register_client failed!");
        }
        info("IPC client: thread %d IPC client registered.\n", tid);

        // TODO: cannot be removed ecause of the stupid impl @lwt
        ipc_msg = ipc_create_msg(client_ipc_struct, 0);
        se = (struct server_exit_msg *)ipc_get_msg_data(ipc_msg);
        se->server_exit_flag = 0;

        for (int i = 0; i < IPC_NUM; i++) {
                ret = ipc_call(client_ipc_struct, ipc_msg);
                chcore_assert(ret == 0, "ipc call failed!");
        }

        ipc_destroy_msg(ipc_msg);

        info("IPC client: thread %d finished.\n", tid);

        return NULL;
}

void invalid_ipc_test(ipc_struct_t *client_ipc_struct)
{
        int ret;
        ipc_msg_t *ipc_msg;
        struct server_exit_msg *se;

        ipc_msg = ipc_create_msg(client_ipc_struct, 0);
        ipc_set_msg_send_cap_num(ipc_msg, 10000);
        se = (struct server_exit_msg *)ipc_get_msg_data(ipc_msg);
        se->server_exit_flag = 0;

        ret = usys_ipc_call(client_ipc_struct->conn_cap,
                            10000);
        chcore_assert(ret == -EINVAL, "invalid ipc call return check failed!");
        ipc_destroy_msg(ipc_msg);

        info("IPC client: invalid ipc_msg tests passed!\n");
}

int main(int argc, char *argv[])
{
        cpu_set_t mask;
        char *args[2];
        struct new_process_caps new_process_caps;
        int thread_num = 32, ret;
        pthread_t thread_id[32];
        ipc_struct_t *client_ipc_struct;
        ipc_msg_t *ipc_msg;
        struct server_exit_msg *se;

        CPU_ZERO(&mask);
        CPU_SET(2 % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        args[0] = "test_ipc_server.bin";
        args[1] = "TOKEN";
        ret = create_process(2, args, &new_process_caps);
        chcore_assert(ret > 0, "create ipc server process failed!");
        server_cap = new_process_caps.mt_cap;

        info("Start testing concurrent IPC.\n");

        if (argc >= 2)
                thread_num = atoi(argv[1]);

        for (int i = 0; i < thread_num; ++i) {
                ret = pthread_create(&thread_id[i],
                               NULL,
                               perf_ipc_routine,
                               (void *)(unsigned long)i);
                chcore_assert(ret == 0, "create ipc thread failed!");
        }

        info("IPC client: create %d threads done.\n", thread_num);

        for (int i = 0; i < thread_num; ++i) {
                ret = pthread_join(thread_id[i], NULL);
                chcore_assert(ret == 0, "ipc thread join failed!");
        }

        info("IPC client: all %d threads finished.\n", thread_num);

        client_ipc_struct = ipc_register_client(server_cap);
        chcore_assert(client_ipc_struct != NULL, "ipc_register_client failed!");
        
        invalid_ipc_test(client_ipc_struct);

        ipc_msg = ipc_create_msg(client_ipc_struct, 1);
        se = (struct server_exit_msg *)ipc_get_msg_data(ipc_msg);
        se->server_exit_flag = 1;
        ret = ipc_call(client_ipc_struct, ipc_msg);
        chcore_assert(ret == 0, "final ipc call failed!");

        ipc_destroy_msg(ipc_msg);

        green_info("Test IPC finished!\n");

        return 0;
}
