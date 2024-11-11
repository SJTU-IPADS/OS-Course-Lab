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

void init(void)
{
        unlinkat(0, "/home/vbn/aaabbb.txt", 0);
        unlinkat(0, "/home/zxc", AT_REMOVEDIR);
        unlinkat(0, "/home/vbn", AT_REMOVEDIR);
        unlinkat(0, "/home/bbbbbb.txt", 0);
        unlinkat(0, "/home/cccccc.txt", 0);
        unlinkat(0, "/home/dddddd.txt", 0);
        unlinkat(0, "/home/eeeeee.txt", 0);
}

int main()
{
        int i;
        int fd[5];
        char write_buf[1024] = {0};
        char read_buf[1024] = {0};
        struct stat stat_buf;

        mount("", "/home", 0, 0, 0);
        init();

        fd[0] = openat(0, "/home/aaaaaa.txt", O_CREAT, 0);
        fd[1] = openat(0, "/home/bbbbbb.txt", O_CREAT, 0);
        fd[2] = openat(0, "/home/cccccc.txt", O_CREAT, 0);
        fd[3] = openat(0, "/home/dddddd.txt", O_CREAT, 0);
        fd[4] = openat(0, "/home/eeeeee.txt", O_CREAT, 0);

        mkdirat(AT_FDROOT, "/home/zxc", 0);
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

        stat("/home/aaaaaa.txt", &stat_buf);
        printf("file size: %d\n", stat_buf.st_size);

        renameat(0, "/home/zxc", 0, "/home/vbn");
        renameat(0, "/home/aaaaaa.txt", 0, "/home/vbn/aaabbb.txt");

        printf("Test finished!\n");
        return 0;
}