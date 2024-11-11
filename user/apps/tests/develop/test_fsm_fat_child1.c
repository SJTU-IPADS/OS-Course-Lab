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
#include <chcore/fs_defs.h>

#include "test_tools.h"

void init(void)
{
        unlinkat(0, "/home/aaa.txt", 0);
}

int main()
{
        int fd;
        int i;
        char write_buff[100] = {0};
        char read_buff[100] = {0};

        init();
        fd = openat(0, "/home/aaa.txt", O_CREAT, 0);

        for (i = 0; i < 99; i++) {
                write_buff[i] = 'A' + i % 26;
        }
        write(fd, write_buff, 99);

        lseek(fd, 0, 0);
        read(fd, read_buff, 99);
        printf("buf 1:%s\n", read_buff);

        close(fd);

        fd = openat(0, "/home/config.txt", O_RDWR, 0);
        lseek(fd, 3, 0);
        read(fd, read_buff, 5);
        read_buff[5] = 0;

        printf("buf 2:%s\n", read_buff);

        close(fd);

        printf("Child thread test finished!\n");
        return 0;
}