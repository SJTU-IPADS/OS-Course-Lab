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
        memcpy(fat32_req->path, "abc.txt", 8);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "abcdefghijk.txt", 16);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "new_text1.txt", 14);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "/abcdefg/text3.txt", 19);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "/abcdefg/text4.txt", 19);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "/abcdefg/text5.txt", 19);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "/abcdefg/text6.txt", 19);
        ipc_call(fat32_ipc_struct, fat32_ipc_msg);

        memcpy(fat32_req->path, "abcdefg", 8);
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
        init_test();
        printf("Test Single file\n");
        {
                int i;
                int ret;

                printf("Test 1...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ;
                memcpy(fat32_req->path, "/config.txt", 12);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 10;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                if (strcmp("enable_uar", fat32_req->buff)) {
                        printf("Read compare error!\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 100;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_WRITE;
                fat32_req->size = 10;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                printf("Write denied:%d\n", ret); // TODO: add a definition

                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                printf("Test 1 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 2...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_WRITE | FA_READ;
                memcpy(fat32_req->path, "abc.txt", 8);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_NO_FILE) {
                        printf("Return value error! ret : %d\n", ret);
                        goto fail_out;
                }
                printf("Test 2 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 3...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ | FA_WRITE | FA_CREATE_NEW;
                memcpy(fat32_req->path, "abc.txt", 8);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                printf("Test 3 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 4...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ | FA_WRITE | FA_CREATE_NEW;
                memcpy(fat32_req->path, "abcdefghijk.txt", 16);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_WRITE;
                fat32_req->size = 1000;
                for (i = 0; i < 1000; i++) {
                        fat32_req->buff[i] = 'a' + (i % 26);
                }
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }

                printf("Stage 1\n");
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ | FA_WRITE;
                memcpy(fat32_req->path, "abcdefghijk.txt", 16);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!Ret:%d\n", ret);
                        goto fail_out;
                }
                for (i = 0; i < 1000; i++) {
                        fat32_req->buff[i] = 0;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 1000;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 500;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                if (fat32_req->buff[0] != 0) {
                        printf("Lseek1 read error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 1000;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_TRUNCATE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_STAT;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (fat32_req->fileinfo.fsize != 1000) {
                        printf("Truncate1 error, file size:%d\n",
                               fat32_req->fileinfo.fsize);
                        goto fail_out;
                }

                printf("Stage 2\n");
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ | FA_WRITE;
                memcpy(fat32_req->path, "abcdefghijk.txt", 16);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!Ret:%d\n", ret);
                        goto fail_out;
                }
                for (i = 0; i < 1000; i++) {
                        fat32_req->buff[i] = 0;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 26;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 500;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                if (fat32_req->buff[0] != 'a') {
                        printf("Lseek2 read error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 26;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_TRUNCATE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_STAT;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (fat32_req->fileinfo.fsize != 26) {
                        printf("Truncate2 error, file size:%d\n",
                               fat32_req->fileinfo.fsize);
                        goto fail_out;
                }

                printf("Stage 3\n");
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_READ | FA_WRITE;
                memcpy(fat32_req->path, "abcdefghijk.txt", 16);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!Ret:%d\n", ret);
                        goto fail_out;
                }
                for (i = 0; i < 1000; i++) {
                        fat32_req->buff[i] = 0;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 0;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 500;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                if (fat32_req->buff[0] != 'a') {
                        printf("Lseek3 read error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 0;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_TRUNCATE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_STAT;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (fat32_req->fileinfo.fsize != 0) {
                        printf("Truncate3 error, file size:%d\n",
                               fat32_req->fileinfo.fsize);
                        goto fail_out;
                }
                printf("Test 4 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 5...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_MKDIR;
                memcpy(fat32_req->path, "abcdefg", 8);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error! ret : %d\n", ret);
                        goto fail_out;
                }
                printf("Test 5 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 6...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_UNLINK;
                memcpy(fat32_req->path, "abc.txt", 8);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Return value error!\n");
                        goto fail_out;
                }
                printf("Test 6 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 7...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_MKDIR;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_EXIST) {
                        printf("Mkdir null path ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_READ;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_OBJECT) {
                        printf("Read invalid fd ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_CLOSE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_OBJECT) {
                        printf("Close invalid fd ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_WRITE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_OBJECT) {
                        printf("Write invalid fd ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_LSEEK;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_OBJECT) {
                        printf("Lseek invalid fd ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_TRUNCATE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_OBJECT) {
                        printf("Truncate invalid fd ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_UNLINK;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_NAME) {
                        printf("Delete null path ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_STAT;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_INVALID_NAME) {
                        printf("Get stat with null path ret error\n");
                        goto fail_out;
                }

                fat32_req->req = FAT32_REQ_STAT;
                memcpy(fat32_req->path, "abcdef", 7);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_NO_FILE) {
                        printf("Get stat with wrong path ret error\n");
                        goto fail_out;
                }
                printf("Test 7 : \033[1;32mPASS\033[0m\n\n");

                printf("\033[1;32mSingle test passed!\033[0m\n\n");
        }

        {
#define FILE_NUM 6
                printf("Multi file\n");

                int i, ret;
                int fd[FILE_NUM];
                char name[FILE_NUM][19] = {
                        "/abcdefg/text1.txt",
                        "/abcdefg/text2.txt",
                        "/abcdefg/text3.txt",
                        "/abcdefg/text4.txt",
                        "/abcdefg/text5.txt",
                        "/abcdefg/text6.txt",
                };
                // char *name1 = "text1.txt";
                // char *name2 = "text2.txt";
                // char *name3 = "text3.txt";
                // char *name4 = "text4.txt";
                // char *name5 = "text5.txt";
                // char *name6 = "text6.txt";

                printf("Test 1 : ");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_WRITE | FA_READ | FA_CREATE_NEW;
                for (i = 0; i < FILE_NUM; i++) {
                        memcpy(fat32_req->path, name[i], 19);
                        ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                        if (ret != FR_OK) {
                                printf("Create no.%d file return error! ret : %d\n",
                                       i,
                                       ret);
                                goto fail_out;
                        }
                        fd[i] = fat32_req->fd;
                }

                for (i = 0; i < FILE_NUM; i++) {
                        int j;
                        fatmsg_clear(fat32_req);
                        fat32_req->req = FAT32_REQ_WRITE;
                        fat32_req->fd = fd[i];
                        fat32_req->size = 100;
                        for (j = 0; j < 100; j++) {
                                fat32_req->buff[j] = 'a' + (j % 26);
                        }
                        ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                        if (ret != FR_OK) {
                                printf("Write no.%d file return error! ret : %d\n",
                                       i,
                                       ret);
                                goto fail_out;
                        }
                }

                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_CLOSE;
                for (i = 5; i >= 0; i--) {
                        fat32_req->fd = fd[i];
                        ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                        if (ret != FR_OK) {
                                printf("Close no.%d file return error! ret : %d\n",
                                       i,
                                       ret);
                                goto fail_out;
                        }
                }
                printf("Test 1 : \033[1;32mPASS\033[0m\n\n");

                printf("Test 2 : \n");
                fat32_req->req = FAT32_REQ_OPEN;
                fat32_req->mode = FA_WRITE | FA_READ;
                for (i = 0; i < 6; i++) {
                        memcpy(fat32_req->path, name[i], 19);
                        ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                        if (ret != FR_OK) {
                                printf("Open no.%d file return error! ret : %d\n",
                                       i,
                                       ret);
                                goto fail_out;
                        }
                        fd[i] = fat32_req->fd;
                }

                printf("Operation 1...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_WRITE;
                fat32_req->size = 1;
                fat32_req->buff[0] = 'A';
                fat32_req->fd = fd[3];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Write file return error! ret : %d\n", ret);
                        goto fail_out;
                }

                printf("Operation 2...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_READ;
                fat32_req->size = 100;
                fat32_req->fd = fd[4];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Write file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                printf("Read data:\n%s\n", fat32_req->buff);

                printf("Operation 3...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_LSEEK;
                fat32_req->ofs = 50;
                fat32_req->fd = fd[5];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Lseek file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_TRUNCATE;
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Truncate file return error! ret : %d\n", ret);
                        goto fail_out;
                }

                printf("Operation 4...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_CLOSE;
                fat32_req->fd = fd[0];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Close file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_RENAME;
                memcpy(fat32_req->path, name[0], 19);
                memcpy(fat32_req->new_path, "/new_text1.txt", 15);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Rename file return error! ret : %d\n", ret);
                        goto fail_out;
                }

                printf("Operation 5...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_CLOSE;
                fat32_req->fd = fd[1];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Close file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_UNLINK;
                memcpy(fat32_req->path, name[1], 19);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Close file return error! ret : %d\n", ret);
                        goto fail_out;
                }

                printf("Operation 6...\n");
                fatmsg_clear(fat32_req);
                fat32_req->req = FAT32_REQ_CLOSE;
                fat32_req->fd = fd[2];
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Close file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                fat32_req->req = FAT32_REQ_STAT;
                memcpy(fat32_req->path, name[2], 19);
                ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                if (ret != FR_OK) {
                        printf("Get stat file return error! ret : %d\n", ret);
                        goto fail_out;
                }
                printf("File name:%s, file size:%d\n",
                       fat32_req->fileinfo.fname,
                       fat32_req->fileinfo.fsize);

                fatmsg_clear(fat32_req);
                for (i = 3; i < 6; i++) {
                        fat32_req->req = FAT32_REQ_CLOSE;
                        fat32_req->fd = fd[i];
                        ret = ipc_call(fat32_ipc_struct, fat32_ipc_msg);
                        if (ret != FR_OK) {
                                printf("Close no.%d file return error! ret : %d\n",
                                       i,
                                       ret);
                                goto fail_out;
                        }
                }
                printf("Test 2 : \033[1;32mPASS\033[0m\n\n");

                printf("\033[1;32mMulti file test passed!\033[0m\n\n");
        }

        goto out;

fail_out:
        printf("Test \033[1;31mFAILED\033[0m\n\n");
out:
        ipc_destroy_msg(fat32_ipc_msg);
        return 0;
}