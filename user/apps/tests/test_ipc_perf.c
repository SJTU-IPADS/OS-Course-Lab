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

#include "test_tools/test_tools.h"
#define _GNU_SOURCE

#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <chcore/type.h>
#include <chcore/syscall.h>
#include <sys/time.h>
#include <inttypes.h>
#include <time.h>
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

#define TEST_TRANSFER_CAP_NUM 15
#define MAGIC_NUM 0xbeef0000

cap_t server_cap;

struct server_msg {
        int msg_flag;
};

long int get_time_us()
{
        int err;
        struct timespec t;

        err = clock_gettime(0, &t);
        chcore_assert(err == 0, "clock_gettime failed!");

        return t.tv_sec*1000000 + t.tv_nsec/1000;
}

void *ipc_test_multi_thread(void *arg)
{
        ipc_struct_t *client_ipc_struct;
        cpu_set_t mask;
        int tid = (int)(long)arg;
        int ret;
        ipc_msg_t *ipc_msg;
        struct server_msg *se;

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

        ipc_msg = ipc_create_msg(client_ipc_struct, 0);
        se = (struct server_msg *)ipc_get_msg_data(ipc_msg);
        se->msg_flag = 0;

        for (int i = 0; i < IPC_NUM; i++) {
                ret = ipc_call(client_ipc_struct, ipc_msg);
                chcore_assert(ret == 0, "ipc call failed!");
        }

        ipc_destroy_msg(ipc_msg);

        return NULL;
}

void ipc_test_cap_transfer(ipc_struct_t *client_ipc_struct, bool ret_cap_flag)
{
        int ret;
        ipc_msg_t *ipc_msg;
        struct server_msg *se;
        cap_t pmo_caps[TEST_TRANSFER_CAP_NUM];
        unsigned int pmo_msg;
        cap_t ret_cap;
        unsigned int read_val;

        for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                pmo_caps[i] = usys_create_pmo(0x1000, PMO_DATA);
                chcore_assert(pmo_caps[i] >= 0, "fail to create pmo!");
                pmo_msg = MAGIC_NUM + (unsigned int)i;
                usys_write_pmo(pmo_caps[i], 0, &pmo_msg, sizeof(pmo_msg));
        }

        ipc_msg = ipc_create_msg_with_cap(client_ipc_struct, 0, TEST_TRANSFER_CAP_NUM);
        for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                ret = ipc_set_msg_cap(ipc_msg, i, pmo_caps[i]);
                if (ret != 0) {
                        error("set msg cap %d %d fail\n", i, pmo_caps[i]);
                        return;
                }
        }

        se = (struct server_msg *)ipc_get_msg_data(ipc_msg);
        if (ret_cap_flag) {
                se->msg_flag = 1;
        } else {
                se->msg_flag = 2;
        }

        ret = ipc_call(client_ipc_struct, ipc_msg);
        chcore_assert(ret != -EINVAL, "ipc call with large buffer return check failed!");

        if (ret_cap_flag) {
                chcore_assert(ipc_get_msg_return_cap_num(ipc_msg) == TEST_TRANSFER_CAP_NUM, "ipc return cap num wrong!");

                for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                        ret_cap = ipc_get_msg_cap(ipc_msg, i);
                        usys_read_pmo(ret_cap, 0, &read_val, sizeof(read_val));
                        chcore_assert(read_val = MAGIC_NUM + (unsigned int)(i + 1), "ipc cap return value wrong");
                }
        } else {
                for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                        usys_read_pmo(pmo_caps[i], 0, &read_val, sizeof(read_val));
                        chcore_assert(read_val = MAGIC_NUM + (unsigned int)(i + 1), "ipc cap return value wrong");
                }
        }

        ipc_destroy_msg(ipc_msg);
}


int main(int argc, char *argv[])
{
        cpu_set_t mask;
        char *args[2];
        struct new_process_caps new_process_caps;
        int thread_num;
        int ret;
        int test_loop;
        u64 start_time;
        u64 end_time;
        pthread_t thread_id[32];
        ipc_struct_t *client_ipc_struct;
        ipc_msg_t *ipc_msg;
        struct server_msg *se;

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

        {
                start_time = pmu_read_real_cycle();

                thread_num = 32;
                for (int i = 0; i < thread_num; ++i) {
                        ret = pthread_create(&thread_id[i],
                                       NULL,
                                       ipc_test_multi_thread,
                                       (void *)(unsigned long)i);
                        chcore_assert(ret == 0, "create ipc thread failed!");
                }

                for (int i = 0; i < thread_num; ++i) {
                        ret = pthread_join(thread_id[i], NULL);
                        chcore_assert(ret == 0, "ipc thread join failed!");
                }

                end_time = pmu_read_real_cycle();
                info("test ipc with %d threads, time: %lu cycles\n", thread_num, end_time - start_time);
        }

        client_ipc_struct = ipc_register_client(server_cap);
        chcore_assert(client_ipc_struct != NULL, "ipc_register_client failed!");
        test_loop = 100;

        {
                start_time = pmu_read_real_cycle();
                for (int i = 0; i < test_loop; i++) {
                        ipc_test_cap_transfer(client_ipc_struct, false);
                }
                end_time = pmu_read_real_cycle();
                info("test ipc with send cap, loop: %d, time: %lu cycles\n", test_loop, end_time - start_time);
        }

        {
                start_time = pmu_read_real_cycle();
                for (int i = 0; i < test_loop; i++) {
                        ipc_test_cap_transfer(client_ipc_struct, true);
                }
                end_time = pmu_read_real_cycle();
                info("test ipc with send cap and return cap, loop: %d, time: %lu cycles\n", test_loop, end_time - start_time);
        }

        {
                ipc_msg = ipc_create_msg(client_ipc_struct, 1);
                se = (struct server_msg *)ipc_get_msg_data(ipc_msg);
                se->msg_flag = -1;
                ret = ipc_call(client_ipc_struct, ipc_msg);
                chcore_assert(ret == 0, "final ipc call failed!");

                ipc_destroy_msg(ipc_msg);
        }

        green_info("Test IPC Perf finished!\n");

        return 0;
}

