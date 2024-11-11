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
#include <chcore/pmu.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define NET_BUFSIZE    4096
#define NET_SERVERPORT 9000

#if (defined CHCORE_PLAT_BM3823) \
        || ((defined CHCORE_PLAT_LEON3) && (defined CHCORE_QEMU))
#define PLAT_CPU_NUM 1
#else
#define PLAT_CPU_NUM 4
#endif

#define PREFIX  "[TEST]"
#define EPREFIX "[ERROR]"

#define COLOR_DEFAULT   "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_YELLOW_BG "\033[33;3m"

#define __print(color, prefix, fmt, ...) \
            printf(color prefix " " fmt COLOR_DEFAULT, ##__VA_ARGS__)
#define info(fmt, ...)  __print(COLOR_DEFAULT, PREFIX, fmt, ##__VA_ARGS__)
#define error(fmt, ...) __print(COLOR_RED, EPREFIX, fmt, ##__VA_ARGS__)
#define green_info(fmt, ...) __print(COLOR_GREEN, PREFIX, fmt, ##__VA_ARGS__)

#define chcore_assert(test, message, ...)                       \
        if (!(test)) {                                          \
                error("Assertion failed: %s (%s: %s: %d), "     \
                      message "\n",                             \
                      #test,                                    \
                      __FILE__,                                 \
                      __func__,                                 \
                      __LINE__,                                 \
                      ##__VA_ARGS__);                           \
                exit(1);                                        \
        }

void delay(void);
int write_log(char *filepath, const char *content);
int expect_log(char *filepath, const char *content);
#if !__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#define __atomic_fetch_add(addr, inc, dummy) \
        ({ ___atomic_fetch_add(addr, inc); })
#define __atomic_add_fetch(addr, inc, dummy) \
        ({ ___atomic_fetch_add(addr, inc) + inc; })

int ___atomic_fetch_add(volatile int *addr, int inc);
#endif
