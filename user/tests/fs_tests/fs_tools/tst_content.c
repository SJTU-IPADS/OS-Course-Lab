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

static char *sub_dir = "for_test_content";
char *test_content_dir;

void test_content_small(void)
{
        char *fname;
        int fd;
        int ret;
        size_t size;
        off_t off;
        char *buf;
        char *buf2;
        char large_buffer[1 << 10];

        fname = path_join(test_content_dir, "for_test_content_small.txt");

        fs_assert_noent(fname);
        fd = open(fname, O_CREAT | O_RDWR);
        fs_assert(fd > 0);
        fs_assert_empty_reg(fname);

        buf = "oiwajefoijopsdfkjwoeijbvohzoheoqpnbvoe";
        size = write(fd, buf, strlen(buf));
        fs_test_eq(size, strlen(buf));
        fs_assert_reg_size(fname, strlen(buf));

        off = lseek(fd, 0, SEEK_SET);
        fs_test_eq(off, 0);
        size = read(fd, large_buffer, 512);
        fs_test_eq(size, strlen(buf));
        fs_assert_reg_size(fname, strlen(buf));
        fs_assert_buffer_eq(buf, large_buffer, size);

        buf2 = "ijweoifji";
        off = lseek(fd, 5, SEEK_SET);
        fs_test_eq(off, 5);
        size = write(fd, buf2, strlen(buf2));
        fs_test_eq(size, strlen(buf2));
        fs_assert_reg_size(fname, strlen(buf));

        ret = close(fd);
        fs_assert_zero(ret);
        fd = open(fname, O_RDWR);
        fs_assert(fd > 0);

        size = read(fd, large_buffer, 512);
        fs_test_eq(size, strlen(buf));
        fs_assert_buffer_eq(large_buffer, buf, 5);
        fs_assert_buffer_eq(large_buffer + 5, buf2, strlen(buf2));
        fs_assert_buffer_eq(large_buffer + 5 + strlen(buf2),
                            buf + 5 + strlen(buf2),
                            strlen(buf) - strlen(buf2) - 5);

        ret = close(fd);
        fs_assert_zero(ret);
        ret = unlink(fname);
        fs_assert_zero(ret);
        fs_assert_noent(fname);
}

void test_content(void)
{
        int ret;

        /* Prepare */
        test_content_dir = sub_dir;
        printf("\ntest_content begin at : %s\n", test_content_dir);

        fs_assert_noent(test_content_dir);
        ret = mkdir(test_content_dir, 0);
        fs_assert_zero(ret);

        /* Test Start */
        test_content_small();

        /* Finish */
        ret = rmdir(test_content_dir);
        fs_assert_zero(ret);
        fs_assert_noent(test_content_dir);

        printf("\ntest_content finished\n");
}