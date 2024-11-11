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
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <time.h>
#include <chcore/fs_defs.h>

#include "test_tools.h"

#define BLOCK_COUNT 30000

void init()
{
        unlinkat(0, "/home/aaa.txt", 0);
}

int main()
{
        int fd;
        int start_time = 0;
        int end_time = 0;
        double total_time;
        char *write_buff = (char *)malloc(BLOCK_COUNT * 512);
        char *read_buff = (char *)malloc(BLOCK_COUNT * 512);

        mount(0, "/home", 0, 0, 0);
        init();
        fd = openat(0, "/home/aaa.txt", O_CREAT, 0);

        start_time = clock();
        write(fd, write_buff, BLOCK_COUNT * 512);
        close(fd);
        end_time = clock();
        total_time = ((double)(end_time - start_time)) / 1000000;
        printf("Fat32 write speed: %.3lf KB/s total_time: %.2lfs\n",
               (double)BLOCK_COUNT / 2 / total_time,
               total_time);

        fd = openat(0, "/home/aaa.txt", O_RDONLY, 0);
        start_time = clock();
        read(fd, read_buff, BLOCK_COUNT * 512);
        close(fd);
        end_time = clock();
        total_time = ((double)(end_time - start_time)) / 1000000;
        printf("Fat32 read speed: %.3lf KB/s total_time: %.2lfs\n",
               (double)BLOCK_COUNT / 2 / total_time,
               total_time);

        return 0;
}