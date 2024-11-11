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
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <assert.h>
#include <inttypes.h>

#include <chcore/type.h>
#include <chcore/defs.h>
#include <chcore/syscall.h>
#include <chcore/ipc.h>
#include <chcore/proc.h>
#include <chcore/launcher.h>
#include <chcore/pmu.h>

#define IPC_NUM 100
#define SHM_KEY 0xABC

int years = 70;

int main(int argc, char *argv[], char *envp[])
{
        cap_t new_thread_cap;
        cap_t shared_page_pmo_cap;
        int shared_msg;
        ipc_struct_t *client_ipc_struct;
        ipc_msg_t *ipc_msg;
        int i;
        s64 start_cycle = 0, end_cycle = 0;

        char *args[1];
        struct new_process_caps new_process_caps;

#if 0
        int ret = 0;
        /* For testing shmxxx */
        int shmid;
        void *shmaddr;

        shmid = shmget(SHM_KEY, 0x1000, IPC_CREAT | IPC_EXCL);
        printf("[client] shmget returns shmid %d\n", shmid);
        if (shmid < 0) {
                printf("EXISTING SHM_KEY\n");
                return -1;
        } else {
                shmaddr = shmat(shmid, 0, 0);
                printf("[client] shmat returns addr %p\n", shmaddr);
                *(u64 *)shmaddr = 0xaabbcc;
        }
#endif

        printf("Hello World from user space: %d years!\n", years);

        /* create a new process */
        printf("create the server process.\n");

        args[0] = "/server.bin";
        create_process(1, args, &new_process_caps);
        new_thread_cap = new_process_caps.mt_cap;

        printf("The main thread cap of the newly created process is %d\n",
               new_thread_cap);

        /* register IPC client */
        client_ipc_struct = ipc_register_client(new_thread_cap);
        if (client_ipc_struct == NULL) {
                printf("ipc_register_client failed\n");
                usys_exit(-1);
        }

/* IPC send cap */
#define PAGE_SIZE 0x1000
        shared_page_pmo_cap = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        shared_msg = 0xbeefbeef;
        usys_write_pmo(shared_page_pmo_cap, 0, &shared_msg, sizeof(shared_msg));

        ipc_msg = ipc_create_msg_with_cap(client_ipc_struct, 0, 1);
        ipc_set_msg_cap(ipc_msg, 0, shared_page_pmo_cap);
        ipc_call(client_ipc_struct, ipc_msg);

        if (ipc_get_msg_return_cap_num(ipc_msg) == 1) {
                cap_t ret_cap;
                int read_val;
                ret_cap = ipc_get_msg_cap(ipc_msg, 0);
                printf("cap return %d\n", ret_cap);
                usys_read_pmo(ret_cap, 0, &read_val, sizeof(read_val));
                printf("read pmo from server, val:%x\n", read_val);
        }

        ipc_destroy_msg(ipc_msg);

        /* IPC send data */
        ipc_msg = ipc_create_msg(client_ipc_struct, 4);
        pmu_clear_cnt();
        start_cycle = pmu_read_real_cycle();
        for (i = 0; i < IPC_NUM; i++) {
                ipc_set_msg_data(ipc_msg, (char *)&i, 0, 4);
                ipc_call(client_ipc_struct, ipc_msg);
        }
        end_cycle = pmu_read_real_cycle();
        printf("cycles for %d roundtrips are %" PRId64 ", average = %" PRId64
               "\n",
               IPC_NUM,
               end_cycle - start_cycle,
               (end_cycle - start_cycle) / IPC_NUM);
        ipc_destroy_msg(ipc_msg);

#if 0
        ret = shmdt(shmaddr);
        assert(ret == 0);
        /* Prevent compiler warning/error when release building (assert(x) -> 0) */
        unused(ret);
#endif

        printf("[User] exit now. Bye!\n");

        return 0;
}
