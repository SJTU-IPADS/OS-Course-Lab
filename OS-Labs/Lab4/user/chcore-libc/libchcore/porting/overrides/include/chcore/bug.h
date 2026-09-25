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

#include <stdio.h>

#define BUG_ON(expr)                                                          \
        do {                                                                  \
                if ((expr)) {                                                 \
                        printf("BUG: %s:%d %s\n", __func__, __LINE__, #expr); \
                        for (;;) {                                            \
                        }                                                     \
                }                                                             \
        } while (0)

#define BUG(str)                                                    \
        do {                                                        \
                printf("BUG: %s:%d %s\n", __func__, __LINE__, str); \
                for (;;) {                                          \
                }                                                   \
        } while (0)

#define WARN(msg) printf("WARN: %s:%d %s\n", __func__, __LINE__, msg)

#define WARN_ON(cond, msg)                                      \
        do {                                                    \
                if ((cond)) {                                   \
                        printf("WARN: %s:%d %s on " #cond "\n", \
                               __func__,                        \
                               __LINE__,                        \
                               msg);                            \
                }                                               \
        } while (0)

#define fail_cond(cond, fmt, ...)           \
        do {                                \
                if (!(cond))                \
                        break;              \
                printf(fmt, ##__VA_ARGS__); \
                usys_exit(-1);              \
        } while (0)
