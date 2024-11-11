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
#include <chcore/fs_defs.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "test_tools.h"

#define WRITE_TIMES 30000
#define DEBUG       printf("[DEBUG] %s:%d \n", __FILE__, __LINE__)

int main()
{
        cap_t ext4_server_cap;
        struct proc_request *proc_req;
        struct fs_request *fs_req;
        ipc_msg_t *proc_ipc_msg, *ext4_ipc_msg;
        ipc_struct_t *ext4_ipc_struct;

        proc_ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(proc_req->get_service_cap.service_name, SERVER_EXT4_FS, SERVICE_NAME_LEN);

        ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        ext4_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);

        ext4_ipc_struct = ipc_register_client(ext4_server_cap);
        ext4_ipc_msg =
                ipc_create_msg(ext4_ipc_struct, sizeof(struct sd_msg));
        fs_req = (struct fs_request *)ipc_get_msg_data(ext4_ipc_msg);

        fs_req->req = FS_REQ_OPEN;
        fs_req->flags = O_RDWR | O_CREAT | O_TRUNC;
        char *path = "/test_sd.txt";
        memcpy(fs_req->path, path, strlen(path) + 1);
        int fd = ipc_call(ext4_ipc_struct, ext4_ipc_msg);
        printf("open fd : %d \n", fd);

        char *write_buf = "this is a test line in test_sd.txt\n";
        fs_req->req = FS_REQ_WRITE;
        fs_req->fd = fd;
        fs_req->count = strlen(write_buf);
        DEBUG;
        memcpy((void *)fs_req + sizeof(struct fs_request),
               write_buf,
               strlen(write_buf) + 1);
        printf("data: %s\n", (void *)fs_req + sizeof(struct fs_request));
        DEBUG;
        int wcnt = ipc_call(ext4_ipc_struct, ext4_ipc_msg);
        DEBUG;
        printf("write count: %d\n", wcnt);

        fs_req->req = FS_REQ_CLOSE;
        fs_req->fd = fd;
        ipc_call(ext4_ipc_struct, ext4_ipc_msg);
        printf("fd %d closed\n", fd);

        printf("test_ext4 end! bye!\n");
        return 0;
}