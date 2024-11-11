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
#include <chcore/fat32_defs.h>
#include <chcore/syscall.h>
#include <chcore/proc.h>
#include <chcore/procmgr_defs.h>
#include <chcore/ipc.h>
#include <chcore/launcher.h>
#include <string.h>

#include "test_tools.h"

typedef enum {
        FR_OK = 0, /* (0) Succeeded */
        FR_DISK_ERR, /* (1) A hard error occurred in the low level disk I/O
                        layer */
        FR_INT_ERR, /* (2) Assertion failed */
        FR_NOT_READY, /* (3) The physical drive cannot work */
        FR_NO_FILE, /* (4) Could not find the file */
        FR_NO_PATH, /* (5) Could not find the path */
        FR_INVALID_NAME, /* (6) The path name format is invalid */
        FR_DENIED, /* (7) Access denied due to prohibited access or directory
                      full */
        FR_EXIST, /* (8) Access denied due to prohibited access */
        FR_INVALID_OBJECT, /* (9) The file/directory object is invalid */
        FR_WRITE_PROTECTED, /* (10) The physical drive is write protected */
        FR_INVALID_DRIVE, /* (11) The logical drive number is invalid */
        FR_NOT_ENABLED, /* (12) The volume has no work area */
        FR_NO_FILESYSTEM, /* (13) There is no valid FAT volume */
        FR_MKFS_ABORTED, /* (14) The f_mkfs() aborted due to any problem */
        FR_TIMEOUT, /* (15) Could not get a grant to access the volume within
                       defined period */
        FR_LOCKED, /* (16) The operation is rejected according to the file
                      sharing policy */
        FR_NOT_ENOUGH_CORE, /* (17) LFN working buffer could not be allocated */
        FR_TOO_MANY_OPEN_FILES, /* (18) Number of open files > FF_FS_LOCK */
        FR_INVALID_PARAMETER /* (19) Given parameter is invalid */
} FRESULT;

ipc_struct_t *fat32_ipc_struct;
ipc_msg_t *fat32_ipc_msg;
struct fat32_msg *fat32_req;

void init_test(void)
{
        cap_t fat32_server_cap;
        struct proc_request *proc_req;
        ipc_msg_t *proc_ipc_msg;

        proc_ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(proc_req->get_service_cap.service_name, SERVER_FAT32_FS,
                SERVICE_NAME_LEN);

        ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        fat32_server_cap = ipc_get_msg_cap(proc_ipc_msg, 0);

        fat32_ipc_struct = ipc_register_client(fat32_server_cap);
        fat32_ipc_msg =
                ipc_create_msg(fat32_ipc_struct, sizeof(struct fat32_msg));
        fat32_req = (struct fat32_msg *)ipc_get_msg_data(fat32_ipc_msg);

        /*
         * If there exist some files named abc.txt, abcdefg and
         * abcdefghijk.txt,etc, delete it.
         */
        fat32_req->req = FAT32_REQ_UNLINK;
        memcpy(fat32_req->path, "abcdef.txt", 12);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
}

void fatmsg_clear(struct fat32_msg *msg)
{
        int i;

        msg->fd = -1;
        msg->size = 0;
        msg->ofs = 0;
        msg->mode = 0;
        msg->bw = 0;
        msg->br = 0;
        for (i = 0; i < 512 * 6; i++) {
                msg->buff[i] = 0;
        }
        for (i = 0; i < 256; i++) {
                msg->path[i] = 0;
                msg->new_path[i] = 0;
        }
}

int main()
{
        int i;
        char *name = "/abcdef.txt";
        char *com_name = "/config.txt";
        init_test();

        printf("Test 1...\n");
        fatmsg_clear(fat32_req);
        fat32_req->req = FAT32_REQ_OPEN;
        fat32_req->mode = FA_READ;
        memcpy(fat32_req->path, com_name, 12);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        printf("Common file fd:%d\n", fat32_req->fd);

        sched_yield();
        delay();
        delay();

        fat32_req->req = FAT32_REQ_READ;
        fat32_req->size = 2;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        printf("Child 1 read data:%s\n", fat32_req->buff);

        fat32_req->req = FAT32_REQ_CLOSE;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        printf("Test 1 end\n");

        printf("Test 2\n");
        fatmsg_clear(fat32_req);
        fat32_req->req = FAT32_REQ_OPEN;
        fat32_req->mode = FA_CREATE_NEW | FA_READ | FA_WRITE;
        memcpy(fat32_req->path, name, 12);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        fat32_req->req = FAT32_REQ_WRITE;
        fat32_req->size = 200;
        for (i = 0; i < 200; i++) {
                fat32_req->buff[i] = (i % 26) + 'A';
        }
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        fat32_req->req = FAT32_REQ_CLOSE;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        fatmsg_clear(fat32_req);
        fat32_req->req = FAT32_REQ_OPEN;
        fat32_req->mode = FA_READ | FA_WRITE;
        memcpy(fat32_req->path, name, 12);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        fat32_req->req = FAT32_REQ_READ;
        fat32_req->size = 200;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        printf("Read data:\n%s\n", fat32_req->buff);

        fat32_req->req = FAT32_REQ_LSEEK;
        fat32_req->ofs = 100;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        fat32_req->req = FAT32_REQ_TRUNCATE;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        fat32_req->req = FAT32_REQ_CLOSE;
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);
        printf("Test 2 end\n");

        ipc_destroy_msg(fat32_ipc_msg);
        return 0;
}