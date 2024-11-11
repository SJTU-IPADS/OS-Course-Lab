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

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <unistd.h>

#include "test_tools.h"

#define FILE_SIZE       10000
#define PAGE_SIZE       4096
#define INIT_TEST_STR   "1234567890"

void xm(char *p, int len);
void random_char(char *p, int len);

int main(int argc, char *argv[], char *envp[])
{
        int fd, iovcnt, ret = 0;
        char *fmap;
        char buf[256], buf1[FILE_SIZE], buf2[FILE_SIZE];
        struct stat statbuf;
        struct statfs statfsbuf;
        struct iovec iov[2];

        info("Test FS begins...\n");

        /* ----- FS Basic API test ----- */

        ret = openat(2, "/", 0);
        chcore_assert(ret >= 0, "openat failed!");

        ret = open("/", 0, 0);
        chcore_assert(ret >= 0, "open failed!");

        fd = open("/test_hello.bin", 0);
        chcore_assert(ret >= 0, "open failed!");

        ret = close(fd);
        chcore_assert(ret == 0, "close failed!");

        fd = open("/test.txt", O_RDWR);
        chcore_assert(fd >= 0, "open failed!");

        statbuf.st_mode = 0;
        ret = stat("/", &statbuf);
        chcore_assert(ret == 0, "stat failed!");
        chcore_assert(statbuf.st_mode == (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR), "wrong st_mode!");

        statbuf.st_mode = 0;
        ret = lstat("/", &statbuf);
        chcore_assert(ret == 0, "lstat failed!");
        chcore_assert(statbuf.st_mode == (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR), "wrong st_mode!");

        statbuf.st_mode = 0;
        ret = fstatat(AT_FDCWD, "/", &statbuf, 0);
        chcore_assert(ret == 0, "fstatat failed!");
        chcore_assert(statbuf.st_mode == (S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR), "wrong st_mode!");

        statbuf.st_mode = 0;
        ret = fstat(fd, &statbuf);
        chcore_assert(ret == 0, "fstat failed!");
        chcore_assert(S_ISREG(statbuf.st_mode), "wrong st_mode!");
        
        statfsbuf.f_type = 0;
        ret = statfs("/", &statfsbuf);
        chcore_assert(ret == 0, "statfs failed!");
        chcore_assert(statfsbuf.f_type == 0xCCE7A9F5, "wrong fs type!");

        statfsbuf.f_type = 0;
        ret = fstatfs(fd, &statfsbuf);
        chcore_assert(ret == 0, "fstatfs failed!");
        chcore_assert(statfsbuf.f_type == 0xCCE7A9F5, "wrong fs type!");

        ret = faccessat(-1, "/test.txt", R_OK, 0);
        chcore_assert(ret == 0, "faccessat failed!");

        ret = faccessat(AT_FDCWD, "/no-file", F_OK, 0);
        chcore_assert(ret == -1, "faccessat supposed to fail but not!");

        ret = access("/test_hello.bin", X_OK);
        chcore_assert(ret == 0, "access failed!");

        ret = rename("/test_hello.bin", "/test_hello.bin");
        chcore_assert(ret == 0, "rename failed!");

        ret = read(fd, buf, 10);
        chcore_assert(ret >= 0, "read failed!");
        buf[ret] = '\0';
        info("Read test.txt: %s\n", buf);

        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");

        ret = write(fd, "321", 3);
        chcore_assert(ret == 3, "write failed!");

        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");

        ret = read(fd, buf, 10);
        chcore_assert(ret >= 3, "read failed!");
        buf[ret] = '\0';
        chcore_assert(strncmp("321", buf, 3) == 0,
                "read content doesn't match!");
        info("Read test.txt (after writing \"321\"): %s\n", buf);

        fmap = (char *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
        chcore_assert(fmap != MAP_FAILED, "mmap failed!");
        /* `ret` must be the return value of the last `read` */
        memcpy(buf1, fmap, ret);
        buf1[ret] = '\0';
        chcore_assert(strcmp(buf1, buf) == 0,
                "mmap read content doesn't match!");

        *fmap = '7';
        /* `ret` must be the return value of the last `read` */
        memcpy(buf1, fmap, ret);
        buf1[ret] = '\0';

        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");
        ret = read(fd, buf, 10);
        chcore_assert(ret >= 0, "read failed!");
        buf[ret] = '\0';
        chcore_assert(strcmp(buf1, buf) == 0,
                "mmap read content doesn't match!");

        ret = mkdir("/test_dir", S_IRUSR | S_IWUSR | S_IXUSR);
        chcore_assert(ret == 0, "mkdir failed!");

        ret = rmdir("/test_dir");
        chcore_assert(ret == 0, "rmdir failed!");

        ret = unlink("/test.txt");
        chcore_assert(ret == 0, "unlink failed!");

        fd = open("/test.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        chcore_assert(ret >= 0, "open with O_CREAT failed!");

        random_char(buf1, FILE_SIZE);

        ret = write(fd, buf1, FILE_SIZE);
        chcore_assert(ret == FILE_SIZE, "write large file failed!");

        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");

        ret = read(fd, buf2, FILE_SIZE);
        chcore_assert(ret == FILE_SIZE, "read large file failed!");

        ret = memcmp(buf1, buf2, FILE_SIZE - 1);
        chcore_assert(ret == 0,
                "read/write large file content doesn't match!");

        ret = fcntl(fd, F_DUPFD, 10);
        chcore_assert(ret >= 10, "fcntl failed!");

        ret = unlink("/test.txt");
        chcore_assert(ret == 0, "unlink failed!");

        fd = open("/test.txt", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
        chcore_assert(ret >= 0, "open with O_CREAT failed!");

        ret = write(fd, INIT_TEST_STR, strlen(INIT_TEST_STR));
        chcore_assert(ret == strlen(INIT_TEST_STR), "write failed!");

        ret = close(fd);
        chcore_assert(ret == 0, "close failed!");

        fd = open("/creat.txt", O_CREAT | O_RDWR);
        chcore_assert(fd >= 0, "open with O_CREAT failed!");

        iov[0].iov_base = "test ";
        iov[0].iov_len = strlen("test ");
        iov[1].iov_base = "file system";
        iov[1].iov_len = strlen("file system");
        iovcnt = sizeof(iov) / sizeof(struct iovec);
        ret = writev(fd, iov, iovcnt);
        chcore_assert(ret >= 0, "writev failed!");

        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");

        iov[0].iov_base = buf1;
        iov[0].iov_len = strlen("test ");
        iov[1].iov_base = buf2;
        iov[1].iov_len = 20;
        iovcnt = sizeof(iov) / sizeof(struct iovec);
        ret = readv(fd, iov, iovcnt);
        chcore_assert(ret > 0, "readv failed!");
        buf1[strlen("test ")] = '\0';
        buf2[strlen("file system")] = '\0';
        chcore_assert(strcmp("test ", buf1) == 0,
                "readv content doesn't match!");
        chcore_assert(strcmp("file system", buf2) == 0,
                "readv content doesn't match!");

        ret = close(fd);
        chcore_assert(ret == 0, "close failed!");
        ret = unlink("/creat.txt");
        chcore_assert(ret == 0, "unlink failed!");

        /* ----- FS CWD Test ----- */

        /* Check current word dir */
        chcore_assert(getcwd(buf, 256) != NULL, "getcwd failed!");
        chcore_assert(strcmp("/", buf) == 0,
                "getcwd content doesn't match!");

        /* Create and change to a new dir, and then check */
        ret = mkdir("/newcwd", S_IRUSR | S_IWUSR | S_IXUSR);
        chcore_assert(ret == 0, "mkdir failed!");
        ret = chdir("/newcwd");
        chcore_assert(ret == 0, "chdir failed!");
        chcore_assert(getcwd(buf, 256) != NULL, "getcwd failed!");
        chcore_assert(strcmp("/newcwd", buf) == 0,
                "getcwd content doesn't match!");

        /* Use relative path instead of absolute path */
        ret = mkdir("dir0", S_IRUSR | S_IWUSR | S_IXUSR);
        chcore_assert(ret == 0, "mkdir failed!");
        ret = chdir("dir0");
        chcore_assert(ret == 0, "chdir failed!");
        chcore_assert(getcwd(buf, 256) != NULL, "getcwd failed!");
        chcore_assert(strcmp("/newcwd/dir0", buf) == 0,
                "getcwd content doesn't match!");

        ret = chdir("/");
        chcore_assert(ret == 0, "chdir failed!");
        ret = rmdir("/newcwd/dir0");
        chcore_assert(ret == 0, "rmdir failed!");
        ret = rmdir("/newcwd");
        chcore_assert(ret == 0, "rmdir failed!");

        /* ----- FS Rename Test ----- */

        fd = open("/test1.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        chcore_assert(fd >= 0, "open with O_CREAT failed!");
        ret = write(fd, "321", 3);
        chcore_assert(ret == 3, "write failed!");
        ret = lseek(fd, 0, SEEK_SET);
        chcore_assert(ret == 0, "lseek failed!");
        ret = read(fd, buf, 10);
        chcore_assert(ret >= 0, "read failed!");
        buf[ret] = '\0';
        ret = close(fd);
        chcore_assert(ret == 0, "close failed!");

        chcore_assert(strcmp("321", buf) == 0,
                "read content doesn't match!");
        memset(buf, 0, 10);

        ret = rename("/test1.txt", "/test2.txt");
        chcore_assert(ret == 0, "rename failed!");

        fd = open("/test2.txt", O_RDONLY);
        chcore_assert(fd >= 0, "open failed!");
        ret = lseek(fd, 0, SEEK_CUR);
        chcore_assert(ret == 0, "lseek failed!");
        ret = read(fd, buf, 10);
        chcore_assert(ret >= 0, "read failed!");
        buf[ret] = '\0';
        ret = close(fd);
        chcore_assert(ret == 0, "close failed!");

        chcore_assert(strcmp("321", buf) == 0,
                "read content doesn't match!");

        ret = unlink("/test2.txt");
        chcore_assert(ret == 0, "unlink failed!");

        green_info("Test FS finished!\n");
        return 0;
}

void xm(char *p, int len)
{
        int i;
        for (i = 0; i < len; i++)
                putchar(*(p + i));
        putchar('\n');
}

void random_char(char *p, int len)
{
        int i;
        for (i = 0; i < len; i++)
                *(p + i) = random() % 26 + 0x41;
}
