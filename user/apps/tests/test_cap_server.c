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

struct msg {
        int number;
};

DEFINE_SERVER_HANDLER(server_trampoline)
{
        struct msg *msg;
        cap_t pmo, pmo_copied;
        int number;
        int ret;

        msg = (struct msg *)ipc_get_msg_data(ipc_msg);

        pmo = ipc_get_msg_cap(ipc_msg, 0);
        chcore_assert(pmo >= 0, "ipc_get_msg_cap failed %d\n", pmo);
        
        ret = usys_read_pmo(pmo, 0, &number, sizeof(number));
        chcore_assert(ret >= 0, "usys_read_pmo failed %d\n", pmo);

        chcore_assert(number == msg->number, "read pmo number incorrect\n");

        ret = usys_revoke_cap(pmo, true);
        chcore_assert(ret == -EPERM, "usys_revoke_cap should return -EPERM, ret = %d\n", ret);

        ret = usys_transfer_caps(SELF_CAP, &pmo, 1, &pmo_copied);
        chcore_assert(ret == 0 && pmo_copied == -EPERM, "usys_transfer_caps should return -EPERM, pmo_copied = %d\n", pmo_copied);

        ipc_return(ipc_msg, 0);
}

int main(int argc, char *argv[], char *envp[])
{
        void *token;
        int ret;

        if (argc == 1)
                goto out;

        token = strstr(argv[1], "TEST_CAP");
        if (!token)
                goto out;

        ret = ipc_register_server(server_trampoline, 
                DEFAULT_CLIENT_REGISTER_HANDLER);
        chcore_assert(ret == 0, "register ipc server failed!");

        usys_exit(0);
out:
        info("Run test_cap.bin instead\n");
        return 0;
}
