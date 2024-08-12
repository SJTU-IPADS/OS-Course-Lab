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

#include "fs_test_lib.h"

struct timespec __get_atime(const char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);

        return stat_buf.st_atim;
}

struct timespec __get_mtime(const char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);

        return stat_buf.st_mtim;
}

struct timespec __get_ctime(const char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);

        return stat_buf.st_ctim;
}

ino_t __get_inode(const char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);

        return stat_buf.st_ino;
}

void __fs_assert_noent(char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, -1, _func, _line);
        __fs_test_eq(errno, ENOENT, _func, _line);
}

void __fs_assert_reg_size(char *path, size_t size, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(S_ISREG(stat_buf.st_mode), 1, _func, _line);
        __fs_test_eq(stat_buf.st_size, size, _func, _line);
}

void __fs_assert_is_dir(char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(S_ISDIR(stat_buf.st_mode), 1, _func, _line);
}

void __fs_assert_is_reg(char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(S_ISREG(stat_buf.st_mode), 1, _func, _line);
}

void __fs_assert_is_symlink(char *path, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(S_ISLNK(stat_buf.st_mode), 1, _func, _line);
}

void __fs_check_mode(char *path, mode_t mode, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(stat_buf.st_mode, mode, _func, _line);
}

void __fs_check_nlinks(char *path, nlink_t nlink, const char *_func, int _line)
{
        int ret;
        struct stat stat_buf;

        ret = lstat(path, &stat_buf);
        __fs_test_eq(ret, 0, _func, _line);
        __fs_test_eq(stat_buf.st_nlink, nlink, _func, _line);
}

int __fs_assert_empty_buffer(char *buf, size_t size)
{
        int i;
        for (i = 0; i < size; i++) {
                if (buf[i] != '\0')
                        return -1;
        }
        return 0;
}

int __fs_assert_buffer_eq(char *buf1, char *buf2, size_t size)
{
        int i;
        for (i = 0; i < size; i++) {
                if (buf1[i] != buf2[i])
                        return -1;
        }
        return 0;
}