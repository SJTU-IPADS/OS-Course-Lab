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

#include <stdio.h>
#include <stdlib.h>

#define check_ret(ret)                              \
        do {                                        \
                if ((ret) < 0) {                    \
                        fprintf(stderr,             \
                                "%s failed: %s:%d", \
                                __func__,           \
                                __FILE__,           \
                                __LINE__);          \
                        exit(-1);                   \
                }                                   \
        } while (0)

#define check_ptr(ptr)                              \
        do {                                        \
                if ((ptr) == NULL) {                \
                        fprintf(stderr,             \
                                "%s failed: %s:%d", \
                                __func__,           \
                                __FILE__,           \
                                __LINE__);          \
                        exit(-1);                   \
                }                                   \
        } while (0)
