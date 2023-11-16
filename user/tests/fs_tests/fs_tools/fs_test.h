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

#pragma once

#ifndef FS_TEST_LINUX
#include <chcore/cpio.h>
#include <chcore/defs.h>
#include <chcore/falloc.h>
#include <chcore-internal/fs_defs.h>
#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore/memory.h>
#include <chcore/launcher.h>
#include <chcore/proc.h>
#endif

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/mount.h>
#include <inttypes.h>

// Test Cases
#include "fstest.h"

extern void run_pjdtest();

#undef PAGE_SIZE
#define PAGE_SIZE 0X1000

/* Dependency */
#ifdef FS_TEST_LINUX
#define CHECK_TIME        1
#define CHECK_MODE        0
#define CHECK_NLINK       1
#define CHECK_INODE_FIXED 1
#define ENABLE_CHOWN      1
#define HAVE_FTRUNCATE    1
#define HAVE_TRUNCATE     1
#define HAVE_PERM         0
#define HAVE_SYMLINK      1
#define HAVE_CHFLAGS      0
#else
#define CHECK_TIME        0
#define CHECK_MODE        0
#define CHECK_NLINK       1
#define CHECK_INODE_FIXED 0
#define ENABLE_CHOWN      0
#define HAVE_FTRUNCATE    1
#define HAVE_TRUNCATE     0
#define HAVE_PERM         0
#define HAVE_SYMLINK      0
#define HAVE_CHFLAGS      0
#endif

#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_YELLOW_BG "\033[33;3m"
#define COLOR_DEFAULT   "\033[0m"

#define colored_printf(COLOR, fmt, ...) \
        printf(COLOR fmt COLOR_DEFAULT "", ##__VA_ARGS__)
#define colored_printf_with_lineinfo(COLOR, fmt, ...)                         \
        do {                                                                  \
                colored_printf(                                               \
                        COLOR, "<%s:%s:%d>: ", __FILE__, __func__, __LINE__); \
                printf(fmt, ##__VA_ARGS__);                                   \
        } while (0)
#define fs_test_log(fmt, ...) \
        colored_printf_with_lineinfo("\n" COLOR_GREEN, fmt, ##__VA_ARGS__)

#if 0
#define test_info printf
#else
#define test_info(...) \
        do {           \
        } while (0)
#endif

#define __fs_test_eq(exp1, exp2, _func, _line)                            \
        do {                                                              \
                s64 res1 = (exp1);                                        \
                s64 res2 = (exp2);                                        \
                if (res1 != res2) {                                       \
                        printf(COLOR_RED                                  \
                               "\nassert fail: func=%s,line=%d: %" PRId64 \
                               " != %" PRId64 "\n" COLOR_DEFAULT "",      \
                               _func,                                     \
                               _line,                                     \
                               res1,                                      \
                               res2);                                     \
                        while (1)                                         \
                                ;                                         \
                } else {                                                  \
                        printf(COLOR_GREEN "|" COLOR_DEFAULT "");         \
                }                                                         \
        } while (0)

#define fs_test_eq(exp1, exp2) __fs_test_eq(exp1, exp2, __func__, __LINE__)
#define fs_assert_zero(exp)    fs_test_eq(exp, 0)

#define fs_assert(cond)                                                                 \
        do {                                                                            \
                if (!(cond)) {                                                          \
                        printf(COLOR_RED                                                \
                               "\nassert fail: file=%s,func=%s,line=%d\n" COLOR_DEFAULT \
                               "",                                                      \
                               __FILE__,                                                \
                               __func__,                                                \
                               __LINE__);                                               \
                        while (1)                                                       \
                                ;                                                       \
                } else {                                                                \
                        printf(COLOR_GREEN "|" COLOR_DEFAULT "");                       \
                }                                                                       \
        } while (0)

static inline void print_hex(char *buf, int size, int per_line)
{
        int i;
        for (i = 0; i < size; i++) {
                if (i % per_line == 0) {
                        printf("\n");
                }
                printf("%02x ", buf[i]);
        }
        printf("\n");
}

extern char *base_dir;

char *path_join(const char *dir, const char *file);
char *str_rand(int len);
char *str_plus(char *s1, char *s2);
char *namegen_max(void);
char *dirgen_max(void);
void mkdir_p_dirmax(char *dirmax);
void rmdir_r_dirmax(char *dirmax);