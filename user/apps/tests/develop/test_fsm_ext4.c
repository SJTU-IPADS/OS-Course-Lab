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

int main()
{
        int fd;
        int fd2;

        mount("sda2", "/part2", 0, 0, 0);

        fd = openat(0, "/part2/fsm_file.txt", O_RDWR | O_CREAT | O_TRUNC, 0);
        printf("fd: %d\n", fd);

        char *write_buf = "this is a test content in fsm_file.txt\n";
        write(fd, write_buf, strlen(write_buf));

        fd2 = openat(0, "/part2/fsm_file2.txt", O_RDWR | O_CREAT | O_TRUNC, 0);
        printf("fd2: %d\n", fd2);

        write_buf = "this is a test content in fsm_file2.txt\n";
        write(fd2, write_buf, strlen(write_buf));

        printf("call close(fd)\n");
        close(fd);
        printf("call close(fd) ret \n");
        printf("call close(fd2)\n");
        close(fd2);
        printf("call close(fd2) ret \n");

        /* testcase 3 */
        fd = openat(0, "/part2/extfile.txt", O_RDWR, 0);
        char read_buf[256];
        int rcnt = read(fd, read_buf, 200);
        printf("rcnt=%d, read_buf:%s", rcnt, read_buf);
        close(fd);

        /* testcase 4 */
        fd = openat(0, "/part2/extfile.txt", O_RDWR, 0);
        int off = lseek(fd, 50, SEEK_SET);
        printf("off: %d\n", off);
        write_buf = "seekappend";
        int wcnt = write(fd, write_buf, strlen(write_buf));
        printf("wcnt: %d\n", wcnt);
        close(fd);

        umount("sda2");

        printf("Test finished!\n");
        return 0;
}