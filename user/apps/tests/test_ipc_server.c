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

#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_tools.h"

bool server_exit_flag = false;

#define TEST_TRANSFER_CAP_NUM 15
#define MAGIC_NUM 0xbeef0000

struct server_msg {
        int msg_flag;
};

DEFINE_SERVER_HANDLER(server_trampoline)
{
        cap_t receive_cap;
        cap_t ret_cap;
        unsigned int write_val;
        struct server_msg *se;
        bool ret_with_cap = false;

        se = (struct server_msg *)ipc_get_msg_data(ipc_msg);
        if (se->msg_flag == -1) {
                server_exit_flag = true;
        }

        if (ipc_get_msg_send_cap_num(ipc_msg) == TEST_TRANSFER_CAP_NUM) {
                unsigned int shared_msg;


                for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                        receive_cap = ipc_get_msg_cap(ipc_msg, i);
                        usys_read_pmo(receive_cap, 0, &shared_msg, sizeof(shared_msg));
                        chcore_assert(shared_msg == MAGIC_NUM + i, "ipc message wrong");

                        if (se->msg_flag == 2) {
                                write_val = MAGIC_NUM + i + 1;
                                usys_write_pmo(receive_cap, 0, &write_val, sizeof(write_val));
                        }
                }

                if (se->msg_flag == 1) {
                        ret_with_cap = true;
                        ipc_set_msg_return_cap_num(ipc_msg, TEST_TRANSFER_CAP_NUM);

                        for (int i = 0; i < TEST_TRANSFER_CAP_NUM; i++) {
                                ret_cap = usys_create_pmo(0x1000, PMO_DATA);
                                ipc_set_msg_cap(ipc_msg, i, ret_cap);

                                write_val = MAGIC_NUM + i + 1;
                                usys_write_pmo(ret_cap, 0, &write_val, sizeof(write_val));
                        }
                }
        }

        if (ret_with_cap)
                ipc_return_with_cap(ipc_msg, 0);
        else
                ipc_return(ipc_msg, 0);
}

int main(int argc, char *argv[], char *envp[])
{
        void *token;
        int ret;

        if (argc == 1)
                goto out;

        token = strstr(argv[1], "TOKEN");
        if (!token)
                goto out;

        ret = ipc_register_server(server_trampoline, 
                DEFAULT_CLIENT_REGISTER_HANDLER);
        chcore_assert(ret == 0, "register ipc server failed!");

        while (!server_exit_flag)
                usys_yield();
        exit(0);
out:
        info("IPC server: no IPC client. Bye!\n");
        return 0;
}
