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

static char *sub_dir = "for_test_lseek";
char *test_lseek_dir;

/**
 * Basic test for lseek
 */
static void test_lseek_basic(void)
{
        char *fname;
        int fd;
        int ret;
        off_t off;
        size_t size;
        char page[512];
        char buf[512];

        fname = path_join(test_lseek_dir, "test_lseek_basic.txt");
        fs_assert_noent(fname);

        fd = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd > 0);
        fs_assert_empty_reg(fname);

        memset(page, 0, 512);
        size = write(fd, page, 512);
        fs_test_eq(size, 512);
        fs_assert_reg_size(fname, 512);

        off = lseek(fd, 0, SEEK_SET);
        fs_test_eq(off, 0);
        size = read(fd, buf, 512);
        fs_test_eq(size, 512);
        fs_assert_empty_buffer(buf, 512);
        fs_assert_reg_size(fname, 512);

        off = lseek(fd, -100, SEEK_SET);
        fs_test_eq(off, (off_t)-1);
        fs_test_eq(errno, EINVAL);

        off = lseek(fd, 10, SEEK_SET);
        fs_test_eq(off, 10);
        size = write(fd, "a", 1);
        fs_test_eq(size, 1);

        off = lseek(fd, 40 - 1, SEEK_CUR);
        fs_test_eq(off, 50);
        size = write(fd, "b", 1);
        fs_test_eq(size, 1);

        off = lseek(fd, 20, SEEK_END);
        fs_test_eq(off, 532);
        fs_assert_reg_size(fname, 512);

        off = lseek(fd, -20, SEEK_END);
        fs_test_eq(off, 492);
        size = write(fd, "c", 1);
        fs_test_eq(size, 1);

        off = lseek(fd, 0, SEEK_SET);
        fs_test_eq(off, 0);
        size = read(fd, buf, 512);
        fs_test_eq(size, 512);
        fs_assert_empty_buffer(buf, 10);
        fs_test_eq(buf[10], 'a');
        fs_assert_empty_buffer(buf + 11, 50 - 11);
        fs_test_eq(buf[50], 'b');
        fs_assert_empty_buffer(buf + 51, 492 - 51);
        fs_test_eq(buf[492], 'c');
        fs_assert_empty_buffer(buf + 493, 512 - 493);

        ret = close(fd);
        fs_assert_zero(ret);
        fs_assert_reg_size(fname, 512);

        ret = unlink(fname);
        fs_assert_zero(ret);
        fs_assert_noent(fname);
}

/**
 * When we call lseek with offset outside the file,
 *      the filesize should not be changed at once.
 * If now we call write with size=0, the size will not be changed yet.
 * When write beyond the outside offset with size!=0,
 *      the hole is filling with zeros and file size finally changed.
 */
static void test_lseek_hole(void)
{
        char *fname;
        int fd;
        int ret;
        off_t off;
        size_t size;

        fname = path_join(test_lseek_dir, "test_lseek_hole.txt");
        fs_assert_noent(fname);

        fd = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        fs_assert(fd > 0);
        fs_assert_empty_reg(fname);

        off = lseek(fd, 24, SEEK_SET);
        fs_test_eq(off, 24);
        fs_assert_empty_reg(fname);

        size = write(fd, "content", 0);
        fs_test_eq(size, 0);
        fs_assert_empty_reg(fname);

        size = write(fd, "content", 7);
        fs_test_eq(size, 7);
        fs_assert_reg_size(fname, 31);

        ret = close(fd);
        fs_assert_zero(ret);

        ret = unlink(fname);
        fs_assert_zero(ret);
        fs_assert_noent(fname);
}

void test_lseek(void)
{
        int ret;

        /* Prepare */
        test_lseek_dir = sub_dir;
        printf("\ntest_lseek begin at : %s\n", test_lseek_dir);

        fs_assert_noent(test_lseek_dir);
        ret = mkdir(test_lseek_dir, S_IRUSR | S_IWUSR | S_IXUSR);
        fs_assert_zero(ret);

        /* Test Start */
        test_lseek_basic();
        test_lseek_hole();

        /* Finish */
        ret = rmdir(test_lseek_dir);
        fs_assert_zero(ret);
        fs_assert_noent(test_lseek_dir);

        printf("\ntest_lseek finished\n");
}
