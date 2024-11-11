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
#include <string.h>

#include "test_tools.h"

cap_t sd_server_cap;
struct proc_request *proc_req;
struct sd_msg *sd_req;
ipc_msg_t *proc_ipc_msg, *sd_ipc_msg;
ipc_struct_t *sd_ipc_struct;

void printx(unsigned char *buffer, unsigned int count)
{
        for (int i = 0; i < count; i++) {
                printf("%x ", buffer[i]);
                if (i % 32 == 31) {
                        printf("\n");
                }
        }
        printf("\n");
}

typedef struct partition_struct {
        unsigned char boot;
        unsigned char head;
        unsigned char sector;
        unsigned char cylinder;
        unsigned char fs_id;
        unsigned char head_end;
        unsigned char sector_end;
        unsigned char cylinder_end;
        unsigned int lba;
        unsigned int total_sector;
} partition_struct_t;

void print_partition2(partition_struct_t *p)
{
        printf("partition infomation:\n");

        // parse boot
        if (p->boot == 0x0) {
                printf("  boot: no\n");
        } else if (p->boot == 0x80) {
                printf("  boot: yes\n");
        } else {
                printf("  boot: (unknown)\n");
        }

        // print fs id
        printf("  file system id: 0x%x\n", p->fs_id);

        // parse fs type
        if (p->fs_id == 0x83) {
                printf("  file system: Linux File System\n");
        } else if (p->fs_id == 0xc) {
                printf("  file system: W95 FAT32 (LBA)\n");
        } else if (p->fs_id == 0) {
                printf("  file system: Unused\n");
        } else {
                printf("  file system: (unknown)\n");
        }

        // parse partition lba
        printf("  partition lba: 0x%x = %d\n", p->lba, p->lba);

        // parse partition size
        printf("  partition size: 0x%x = %d\n",
               p->total_sector,
               p->total_sector);
}

int main()
{
        unsigned char mbr[512];
        partition_struct_t partition_s;

        proc_ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(proc_req->get_service_cap.service_name, SERVER_SD, SERVICE_NAME_LEN);

        ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        sd_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);

        sd_ipc_struct = ipc_register_client(sd_server_cap);
        sd_ipc_msg = ipc_create_msg(sd_ipc_struct, sizeof(struct sd_msg));
        sd_req = (struct sd_msg *)ipc_get_msg_data(sd_ipc_msg);

        sd_req->req = SD_REQ_READ;
        sd_req->op_size = 512;
        sd_req->block_number = 0;
        ipc_call(sd_ipc_struct, sd_ipc_msg);

        printx((unsigned char *)sd_req->pbuffer, 512);

        memcpy(mbr, sd_req->pbuffer, 512);
        unsigned char *pinfo0 = mbr + 446;
        unsigned char *pinfo1 = pinfo0 + 16;
        unsigned char *pinfo2 = pinfo1 + 16;
        unsigned char *pinfo3 = pinfo2 + 16;

        printx(pinfo0, 16);
        printx(pinfo1, 16);
        printx(pinfo2, 16);
        printx(pinfo3, 16);

        printf("sizeof sturct mbr %d\n", sizeof(partition_s));

        print_partition2((partition_struct_t *)pinfo0);
        print_partition2((partition_struct_t *)pinfo1);
        print_partition2((partition_struct_t *)pinfo2);
        print_partition2((partition_struct_t *)pinfo3);

        printf("READ SD MBR ended\n");

        return 0;
}