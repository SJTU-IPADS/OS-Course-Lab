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

#include "fs_test.h"
#include "fs_test_lib.h"
#include "fstest_utils.h"
#define BUFFER_SIZE 512
int do_compare(char buf[])
{
        for (int i = 0; i < BUFFER_SIZE; i++) {
                if (buf[i] != i % 10) {
                        fs_test_log("failed when i=%d, index_tracing info:\n",
                                    i);
                        return -1;
                }
        }
        return 0;
}

void test_dup_general(char *fname1, char *fname2)
{
        int fd1, fd2;
        char read_buf[BUFFER_SIZE];
        memset(read_buf, 0, BUFFER_SIZE);
        fd1 = init_file(fname1, BUFFER_SIZE);
        fd2 = open(fname2, O_RDWR | O_CREAT, 0777);
        fs_assert(fd2 > 0);
        fs_test_eq(dup2(fd1, fd2), fd2);
        fs_test_eq(lseek(fd2, 0, SEEK_SET), 0);
        fs_test_eq(read(fd2, read_buf, BUFFER_SIZE), BUFFER_SIZE);
        fs_test_eq(do_compare(read_buf), 0);
        fs_test_eq(close(fd1), 0);
        fs_test_eq(close(fd2), 0);
        fs_test_eq(unlink(fname1), 0);
        fs_test_eq(unlink(fname2), 0);
        printf("dup general success\n");
        return;
}

void test_dup_stdout(char *fname1)
{
        int fd, tmp_fd; // tmp_fd to restored stdout
        char read_buf[BUFFER_SIZE];
        memset(read_buf, 0, BUFFER_SIZE);
        fd = init_file(fname1, BUFFER_SIZE);
        // Backup standard output
        tmp_fd = dup(STDOUT_FILENO);
        printf("testfd tmpfd %d %d\n", fd, tmp_fd);
        fs_test_eq(dup2(fd, STDOUT_FILENO), STDOUT_FILENO);
        fs_test_eq(lseek(fd, 0, SEEK_SET), 0);
        fs_test_eq(read(fd, read_buf, BUFFER_SIZE), BUFFER_SIZE);
        printf("this line should not print to screen\n");
        // restore standard output
        fs_test_eq(dup2(tmp_fd, STDOUT_FILENO), STDOUT_FILENO);
        fs_test_eq(do_compare(read_buf), 0);
        fs_test_eq(close(fd), 0);
        fs_test_eq(close(tmp_fd), 0);
        fs_test_eq(unlink(fname1), 0);
        fs_assert_noent(fname1);
        printf("stdout success\n");
}

void test_dup_stdin(char *fname1)
{
        printf("dup_stdin start\n");
        int fd, tmp_fd;
        char read_buf[BUFFER_SIZE];
        memset(read_buf, 0, BUFFER_SIZE);
        fd = init_file(fname1, BUFFER_SIZE);
        tmp_fd = dup(STDIN_FILENO);
        printf("testfd tmpfd %d %d\n", fd, tmp_fd);
        fs_test_eq(dup2(fd, STDIN_FILENO), STDIN_FILENO);
        fs_test_eq(lseek(fd, 0, SEEK_SET), 0);
        for (int i = 0; i < BUFFER_SIZE; i++) {
                scanf("%c", &read_buf[i]);
        }
        fs_test_eq(do_compare(read_buf), 0);
        fs_test_eq(dup2(tmp_fd, STDIN_FILENO), STDIN_FILENO);
        fs_test_eq(close(fd), 0);
        fs_test_eq(close(tmp_fd), 0);
        fs_test_eq(unlink(fname1), 0);
        printf("test_dup_stdin success\n");
}

int random_dup(int fd)
{
        int a;
        char read_buf[BUFFER_SIZE];
        memset(read_buf, 0, BUFFER_SIZE);
        srand((unsigned int)clock());
        for (int i = 0; i < READ_NUM; i++) {
                a = rand() % (MAX_FD - fd - 2) + 1;
                if (!(dup2(fd, fd + a) == fd + a))
                        return -1;
                // fd and fd + a share the same offset
                if (!(lseek(fd, 0, SEEK_SET) == 0))
                        return -1;
                if (!(read(fd + a, read_buf, BUFFER_SIZE) == BUFFER_SIZE))
                        return -1;
                if (!(do_compare(read_buf) == 0))
                        return -1;
                if (!(close(fd + a) == 0))
                        return -1;
        }
        return 0;
}
void test_dup_ramdom(char *fname1)
{
        int fd;
        fd = init_file(fname1, BUFFER_SIZE);
        srand((unsigned int)clock());
        fs_test_eq(random_dup(fd), 0);
        fs_test_eq(dup2(fd, fd + MAX_FD), -1);
        fs_test_eq(close(fd), 0);
        fs_test_eq(unlink(fname1), 0);
        printf("test_dup_random finish\n");
}

void test_dup(void)
{
        char fname1[] = "test_dup1.txt", fname2[] = "test_dup2.txt";
        test_dup_general(fname1, fname2);
        test_dup_stdout(fname1);
        test_dup_stdin(fname1);
        test_dup_ramdom(fname1);
        printf("test_dup finish\n");
}
