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

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>
#include <inttypes.h>

#include <chcore/syscall.h>
#include <chcore/defs.h>
#include <chcore/ipc.h>

int years = 70;

DEFINE_SERVER_HANDLER(server_trampoline)
{
        int ret = 0;
        cap_t ret_cap, write_val;
        bool ret_with_cap = false;

        printf("Server: handle IPC with client_badge 0x%x\n", client_badge);

        if (ipc_get_msg_send_cap_num(ipc_msg) == 1) {
                cap_t cap;
                int shared_msg;

                cap = ipc_get_msg_cap(ipc_msg, 0);
                usys_read_pmo(cap, 0, &shared_msg, sizeof(shared_msg));

                /* read from shared memory should be MAGIC_NUM */
                printf("read %x\n", shared_msg);

                /* return a pmo cap */
                ret_cap = usys_create_pmo(0x1000, PMO_DATA);
                if (ret_cap < 0) {
                        printf("usys_create_pmo ret %d\n", ret_cap);
                        usys_exit(ret_cap);
                }
                ipc_set_msg_return_cap_num(ipc_msg, 1);
                ipc_set_msg_cap(ipc_msg, 0, ret_cap);
                ret_with_cap = true;

                write_val = 0x233;
                usys_write_pmo(ret_cap, 0, &write_val, sizeof(write_val));

                ret = 0;
        }

        if (ret_with_cap)
                ipc_return_with_cap(ipc_msg, ret);
        else
                ipc_return(ipc_msg, ret);
}

#define SHM_KEY 0xABC

int main(int argc, char *argv[])
{
#if 0
        int shmid;
        void *shmaddr;

        shmid = shmget(SHM_KEY, 0x1000, IPC_CREAT);
        shmaddr = shmat(shmid, 0, 0);
        printf("[server] shmat returns addr %p\n", shmaddr);
        printf("[server] read shm: get 0x%" PRIx64 " (should be 0xaabbcc)\n",
               *(u64 *)shmaddr);
        assert(*(u64 *)shmaddr == 0xaabbcc);
#endif

        printf("Happy birthday %d years! I am server.\n", years);
        printf("[IPC Server] register server value = %u\n",
               ipc_register_server(server_trampoline,
                                   DEFAULT_CLIENT_REGISTER_HANDLER));

        usys_exit(0);
        return 0;
}
