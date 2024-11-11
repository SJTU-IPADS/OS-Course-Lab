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
#include <stddef.h>
#include <stdlib.h>
#include "sys/stat.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/mman.h>
#include <chcore/defs.h>

#include "test_tools.h"

#define TEST_FILE        "./example.txt"
#define THRESHOLD_64BITS 0x100000000ULL

#define assert_read(fd, size, expect_buf)                   \
        do {                                                \
                void *buf = malloc(size);                   \
                ssize_t ret;                                \
                ret = read(fd, buf, size);                  \
                assert(ret == (size));                      \
                assert(memcmp(buf, expect_buf, size) == 0); \
        } while (0)

#define assert_read_str(fd, expect_str)                   \
        do {                                              \
                ssize_t bufsize = strlen(expect_str);     \
                assert_read((fd), bufsize, (expect_str)); \
        } while (0)

#define assert_file_size(fd, expect_size)              \
        do {                                           \
                struct stat stat;                      \
                int ret;                               \
                ret = fstat(fd, &stat);                \
                assert(ret == 0);                      \
                assert(stat.st_size == (expect_size)); \
        } while (0)

#define assert_write_str(fd, str, expect_file_size)         \
        do {                                                \
                ssize_t ret;                                \
                ret = write(fd, str, strlen(str));          \
                assert(ret == strlen(str));                 \
                assert_file_size((fd), (expect_file_size)); \
        } while (0)

#define assert_set_off(fd, target_off)                 \
        do {                                           \
                off_t ret;                             \
                ret = lseek(fd, target_off, SEEK_SET); \
                assert(ret == (target_off));           \
        } while (0)

#define assert_off(fd, expect_off)                 \
        do {                                           \
                off_t ret;                             \
                ret = lseek(fd, 0, SEEK_CUR); \
                assert(ret == (expect_off));           \
        } while (0)

#define assert_pread_str(fd, expect_str, off) \
        do {                                              \
                ssize_t bufsize = strlen(expect_str);     \
                void *buf = malloc(bufsize);                   \
                ssize_t ret;                                \
                ret = pread(fd, buf, bufsize, off);                  \
                assert(ret == (bufsize));                      \
                assert(memcmp(buf, expect_str, bufsize) == 0); \
        } while (0)

#define assert_pwrite_str(fd, str, off, expect_file_size) \
        do {                                                \
                ssize_t ret;                                \
                ret = pwrite(fd, str, strlen(str), off);                  \
                assert(ret == strlen(str));                      \
                assert_file_size((fd), (expect_file_size)); \
        } while (0)

#define assert_mmapped_content(fd, len, off, expect_buf)                     \
        do {                                                                 \
                void *map_start;                                             \
                size_t map_len = ROUND_UP(len, PAGE_SIZE);                   \
                map_start =                                                  \
                        mmap(NULL, map_len, PROT_READ, MAP_SHARED, fd, off); \
                assert(map_start);                                           \
                assert(memcmp(map_start, expect_buf, len) == 0);             \
        } while (0)

int main(void)
{
        ssize_t ret;
        int fd;
        char buf1[] = "12345";
        char buf2[] = "67890";
        char *zeros = calloc(5, 1);

        info("Test syscall64 begins...\n");

        ret = open(TEST_FILE, O_CREAT | O_RDWR, 0700);
        assert(ret > 0);

        fd = ret;

        ret = ftruncate(fd, THRESHOLD_64BITS);
        assert(ret == 0);

        assert_file_size(fd, THRESHOLD_64BITS);

        assert_write_str(fd, buf1, THRESHOLD_64BITS);

        assert_set_off(fd, THRESHOLD_64BITS);

        assert_write_str(fd, buf2, THRESHOLD_64BITS + strlen(buf2));

        assert_set_off(fd, 0);

        assert_read_str(fd, buf1);

        assert_set_off(fd, THRESHOLD_64BITS);

        assert_read_str(fd, buf2);

        assert_pread_str(fd, buf1, 0);

        assert_off(fd, THRESHOLD_64BITS + strlen(buf2));

        assert_pwrite_str(fd, buf2, strlen(buf1), THRESHOLD_64BITS + strlen(buf2));

        assert_off(fd, THRESHOLD_64BITS + strlen(buf2));

        assert_pread_str(fd, buf2, strlen(buf1));

        assert_off(fd, THRESHOLD_64BITS + strlen(buf2));

        assert_mmapped_content(fd, strlen(buf1), 0, buf1);

        assert_mmapped_content(fd, strlen(buf2), THRESHOLD_64BITS, buf2);

        ret = fallocate(fd,
                        FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
                        0,
                        strlen(buf1));
        assert(ret == 0);

        ret = fallocate(fd,
                        FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
                        THRESHOLD_64BITS,
                        strlen(buf2));
        assert(ret == 0);

        assert_set_off(fd, 0);

        assert_read(fd, 5, zeros);

        assert_set_off(fd, THRESHOLD_64BITS);

        assert_read(fd, 5, zeros);

        close(fd);
        unlink(TEST_FILE);

        green_info("Test syscall64 finished!\n");

        return 0;
}
