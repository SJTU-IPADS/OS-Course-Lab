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

struct msg {
        int number;
};

int main(int argc, char *argv[])
{
        char *args[2];
        struct new_process_caps new_process_caps;
        int ret;
        ipc_struct_t *client_ipc_struct;
        ipc_msg_t *ipc_msg;
        struct msg *msg;
        cap_t pmo;
        cap_t server_cap;

        args[0] = "test_cap_server.bin";
        args[1] = "TEST_CAP";
        ret = create_process(2, args, &new_process_caps);
        chcore_assert(ret > 0, "create ipc server process failed!");
        server_cap = new_process_caps.mt_cap;

        info("Start testing cap.\n");

        /* register mutiple IPC clients */
        client_ipc_struct = ipc_register_client(server_cap);
        chcore_assert(client_ipc_struct != NULL,
                "ipc_register_client failed!");
        info("cap client: IPC client registered.\n");

        ipc_msg = ipc_create_msg(client_ipc_struct, 0);
        msg = (struct msg *)ipc_get_msg_data(ipc_msg);

        pmo = usys_create_pmo(PAGE_SIZE, PMO_ANONYM);
        chcore_assert(pmo >= 0, "usys_create_pmo failed ret %d\n", pmo);
        
        msg->number = 0xdeadbeaf;
        ret = usys_write_pmo(pmo, 0, &msg->number, sizeof(msg->number));

        ret = ipc_set_msg_cap_restrict(ipc_msg,
                                       0,
                                       pmo,
                                       CAP_RIGHT_COPY | CAP_RIGHT_REVOKE_ALL,
                                       CAP_RIGHT_NO_RIGHTS);
        chcore_assert(ret == 0, "ipc_set_msg_cap failed!");
        
        ret = ipc_call(client_ipc_struct, ipc_msg);
        chcore_assert(ret == 0, "ipc call failed!");

        ipc_destroy_msg(ipc_msg);

        green_info("Test cap finished!\n");

        return 0;
}
