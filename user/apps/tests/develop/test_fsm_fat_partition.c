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

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <chcore-internal/fs_defs.h>

int main()
{
        int i;
        int fd[5];
        char write_buf[1024] = {0};
        char read_buf[1024] = {0};

        mount("sda1", "/part1", 0, 0, 0);
        mount("sda3", "/part3", 0, 0, 0);

        fd[0] = openat(0, "/part1/aaaaaa.txt", O_CREAT, 0);
        fd[1] = openat(0, "/part1/bbbbbb.txt", O_CREAT, 0);
        fd[2] = openat(0, "/part3/cccccc.txt", O_CREAT, 0);
        fd[3] = openat(0, "/part3/dddddd.txt", O_CREAT, 0);
        fd[4] = openat(0, "/part3/eeeeee.txt", O_CREAT, 0);
        fd[5] = openat(0, "/part1/test.txt", O_CREAT, 0);

        lseek(fd[5], 100, SEEK_SET);
        close(fd[5]);

        for (i = 0; i < 5; i++) {
                printf("fd[%d] = %d\n", i, fd[i]);
        }

        mkdirat(AT_FDROOT, "/part1/zxc", 0);
        for (i = 0; i < 1023; i++) {
                write_buf[i] = 'a' + i % 26;
        }
        for (i = 0; i < 5; i++) {
                write(fd[i], write_buf, (i + 1) * 100);
        }

        for (i = 0; i < 5; i++) {
                printf("\n");
                lseek(fd[i], -((i + 1) * 99), SEEK_CUR);
                read(fd[i], read_buf, (i + 1) * 100);
                printf("file %d:%s\n", i + 1, read_buf);
        }

        ftruncate(fd[4], 300);

        for (i = 0; i < 5; i++) {
                close(fd[i]);
        }

        printf("Test finished!\n");
        return 0;
}