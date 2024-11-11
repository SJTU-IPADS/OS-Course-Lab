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
/*
 * test_pagecache_perf.c
 * Test pagecache performancr of fat32.
 * 
 * usage:
 * test_pagecache_perf.bin DIR
 * DIR is a mount point of fat32 fs.
 * 
 * man page:
 * Here is an example of how to use this programme in raspi3 qemu.
 * 
 * First prepare a directory which use fat32 fs on linux
 * (Suggest to do these steps under staros root directory):
 * 1. fallocate -l 1G sd.img （File size can customize.）
 * 2. fdisk sd.img
 *    then type n; p; 1; 2048; 1000000; t; c; p; w in turn.
 * 3. sudo losetup --partscan --show --find sd.img
 *    If success, it will show such as "/dev/loop1"
 * 4. sudo fdisk -l
 * 5. sudo mkfs.vfat -F 32 -s 1 /dev/loop1p1
 * 6. sudo losetup -d /dev/loop1
 * 
 * Next, you need to config:
 * 1. turn CHCORE_DRIVER_FWK_CIRCLE on;
 * 2. edit CHCORE_QEMU_SDCARD_IMG to sd.img (path to your img).
 * 
 * Then, run:
 * 1. ./chbuild clean (This is important!)
 * 2. ./chbuild build
 * 
 * Finally, enter into chcore and run:
 * 1. mount.bin sda1 /user
 * 2. test_pagecache_perf.bin /user/
*/

#include <errno.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <chcore-internal/sd_defs.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <chcore-internal/fs_defs.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "test_tools.h"

#define WRITE_TIMES 30000
#define DEBUG       printf("[DEBUG] %s:%d \n", __FILE__, __LINE__)

int main(int argc, char **argv)
{
        cap_t fat32_server_cap;
        struct fs_request *fs_req;
        ipc_msg_t *fat32_ipc_msg, *ipc_msg;
        ipc_struct_t *fat32_ipc_struct;
        struct fsm_request *fsm_req;
        if (argc != 2) {
                printf("argc != 2, see comments in the source code to know how to use.");
                return -1;
        }
        ipc_msg = ipc_create_msg_with_cap(fsm_ipc_struct, sizeof(struct fsm_request), 1);
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);
        strcpy(fsm_req->path, argv[1]);
        fsm_req->req = FSM_REQ_PARSE_PATH;
        ipc_call(fsm_ipc_struct, ipc_msg);
        fat32_server_cap = ipc_get_msg_cap(ipc_msg, 0);
        fat32_ipc_struct = ipc_register_client(fat32_server_cap);
        fat32_ipc_msg =
                ipc_create_msg(fat32_ipc_struct, sizeof(struct fs_request));
        fs_req = (struct fs_request *)ipc_get_msg_data(fat32_ipc_msg);

        fs_req->req = FS_REQ_TEST_PERF;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        return 0;
}