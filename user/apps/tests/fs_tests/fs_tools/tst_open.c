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

static char *sub_dir = "for_test_open";
static char *test_open_dir;

/* TODO: test more open flags */

static void test_open_basic(void)
{
        int fd;
        int ret;
        char *fname;

        fname = path_join(test_open_dir, "test_open.txt");
        fd = open(fname, O_CREAT | O_RDWR);
        fs_assert(fd > 0);
        ret = close(fd);
        fs_assert_zero(ret);
        fs_assert_empty_reg(fname);

        ret = unlink(fname);
        fs_assert_zero(ret);
}

static void test_open_multiple_times(void)
{
        int fd1, fd2;
        int ret;
        char *fname;

        fname = path_join(test_open_dir, "test_open_multiple_times.txt");
        fd1 = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd1 > 0);
        fs_assert_empty_reg(fname);
        fd2 = open(fname, O_RDWR);
        fs_assert(fd2 > 0);
        fs_assert_empty_reg(fname);

        ret = close(fd1);
        fs_assert_zero(ret);
        ret = close(fd2);
        fs_assert_zero(ret);

        ret = unlink(fname);
        fs_assert_zero(ret);
}

static void test_open_no_server_entry_leak(void)
{
        int fd1, fd2;
        int ret;
        char *fname;
        int i;

        fname = path_join(test_open_dir, "test_open_no_server_entry_leak.txt");

        /* create file */
        fd1 = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd1 > 0);
        fs_assert_empty_reg(fname);

        ret = close(fd1);
        fs_assert_zero(ret);

        /* check no server_entry leak */
        for (i = 0; i < 1024 * 16; ++i) {
                fd1 = open(fname, O_RDWR);
                if (fd1 < 0)
                        fs_assert(0);
                fd2 = open(fname, O_RDWR);
                if (fd2 < 0)
                        fs_assert(0);
                ret = close(fd1);
                if (ret != 0)
                        fs_assert(0);
                ret = close(fd2);
                if (ret != 0)
                        fs_assert(0);
        }

        ret = unlink(fname);
        fs_assert_zero(ret);
}

void test_open(void)
{
        int ret;

        test_open_dir = sub_dir;
        printf("\ntest_open begin at : %s\n", test_open_dir);

        fs_assert_noent(test_open_dir);
        ret = mkdir(test_open_dir, S_IRUSR | S_IWUSR | S_IXUSR);
        fs_assert_zero(ret);

        /* Test cases */
        test_open_basic();
        test_open_multiple_times();
        test_open_no_server_entry_leak();

        ret = rmdir(test_open_dir);
        fs_assert_zero(ret);
        fs_assert_noent(test_open_dir);

        printf("\ntest_open finished\n");
}

void test_open_littlefs(void)
{
        int ret;

        test_open_dir = sub_dir;
        printf("\ntest_open begin at : %s\n", test_open_dir);

        fs_assert_noent(test_open_dir);
        ret = mkdir(test_open_dir, S_IRUSR | S_IWUSR | S_IXUSR);
        fs_assert_zero(ret);

        /* Test cases */
        test_open_basic();

        ret = rmdir(test_open_dir);
        fs_assert_zero(ret);
        fs_assert_noent(test_open_dir);

        printf("\ntest_open finished\n");
}
