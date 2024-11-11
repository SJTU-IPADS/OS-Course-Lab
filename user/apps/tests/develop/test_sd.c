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
#include <chcore/sd_defs.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore/procmgr_defs.h>
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <stdlib.h>
#include <time.h>

#include "test_tools.h"

#define WRITE_TIMES 30000

int main()
{
        cap_t sd_server_cap;
        struct proc_request *proc_req;
        struct sd_msg *sd_req;
        ipc_msg_t *proc_ipc_msg, *sd_ipc_msg;
        ipc_struct_t *sd_ipc_struct;
        int i;

        proc_ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP
        strncpy(proc_req->get_service_cap.service_name, SERVER_SD, SERVICE_NAME_LEN);

        ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        sd_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);

        sd_ipc_struct = ipc_register_client(sd_server_cap);
        sd_ipc_msg = ipc_create_msg(sd_ipc_struct, sizeof(struct sd_msg));
        sd_req = (struct sd_msg *)ipc_get_msg_data(sd_ipc_msg);

        sd_req->req = SD_REQ_READ;
        sd_req->op_size = 512;
        for (i = 0; i < 512; i++) {
                sd_req->pbuffer[i] = i;
        }

        {
                sd_req->req = SD_REQ_READ;
                int start_time = clock();

                for (i = 0; i < WRITE_TIMES; i++) {
                        sd_req->block_number = i + 400000;
                        ipc_call(sd_ipc_struct, sd_ipc_msg);
                }

                int end_time = clock();
                double total_time = ((double)(end_time - start_time)) / 1000000;
                double speed = WRITE_TIMES / 2 / total_time;
                printf("rand reading speed:%.3lf KB/s totaltime: %.2lf\n",
                       speed,
                       total_time);
        }

        {
                sd_req->req = SD_REQ_WRITE;
                int start_time = clock();

                for (i = 0; i < WRITE_TIMES; i++) {
                        sd_req->block_number = i + 400000;
                        ipc_call(sd_ipc_struct, sd_ipc_msg);
                }

                int end_time = clock();
                double total_time = ((double)(end_time - start_time)) / 1000000;
                double speed = WRITE_TIMES / 2 / total_time;
                printf("rand writing speed:%.3lf KB/s totaltime: %.2lf\n",
                       speed,
                       total_time);
        }

        printf("test_end!\n");

        printf("Test single block\n");
        printf("Read\n");
        /*Read*/
        ipc_call(sd_ipc_struct, sd_ipc_msg);
        for (i = 0; i < 512; i++) {
                printf("%d ", sd_req->pbuffer[i]);
                if (i % 32 == 31) {
                        printf("\n");
                }
        }

        printf("Write\n");
        /*Write*/
        for (i = 0; i < 26; i++) {
                sd_req->pbuffer[i] = sd_req->pbuffer[i] == i ? 26 - i : i;
        }
        sd_req->req = SD_REQ_WRITE;
        ipc_call(sd_ipc_struct, sd_ipc_msg);

        printf("Read\n");
        /*Read*/
        sd_req->req = SD_REQ_READ;
        ipc_call(sd_ipc_struct, sd_ipc_msg);
        for (int i = 0; i < 512; i++) {
                printf("%d ", sd_req->pbuffer[i]);
                if (i % 32 == 31) {
                        printf("\n");
                }
        }

        printf("Test multi block\n");
        printf("Read\n");
        /*Test multi block option*/
        sd_req->req = SD_REQ_READ;
        sd_req->block_number = 400000;
        ipc_call(sd_ipc_struct, sd_ipc_msg);
        for (i = 0; i < 512 * 6; i++) {
                if (i % 512 == 0) {
                        printf("block:%d\n", i / 512);
                }
                printf("%d ", sd_req->pbuffer[i]);
                if (i % 32 == 31) {
                        printf("\n");
                }
        }

        printf("Write\n");
        /*Write*/
        int j;
        for (j = 0; j < 6; j++) {
                for (i = 0; i < 512; i++) {
                        sd_req->pbuffer[i + j * 512] =
                                sd_req->pbuffer[i] == i ? 512 - i : i;
                }
        }
        sd_req->req = SD_REQ_WRITE;
        ipc_call(sd_ipc_struct, sd_ipc_msg);

        printf("Read\n");
        /*Read*/
        sd_req->req = SD_REQ_READ;
        ipc_call(sd_ipc_struct, sd_ipc_msg);
        for (int i = 0; i < 512 * 6; i++) {
                if (i % 512 == 0) {
                        printf("block:%d\n", i / 512);
                }
                printf("%d ", sd_req->pbuffer[i]);
                if (i % 32 == 31) {
                        printf("\n");
                }
        }

        ipc_destroy_msg(sd_ipc_msg);
        printf("test end!bye!\n");
        return 0;
}