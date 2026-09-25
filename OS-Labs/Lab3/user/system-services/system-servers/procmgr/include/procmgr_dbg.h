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

#ifndef PROCMGR_DBG_H
#define PROCMGR_DBG_H

#include <assert.h>
#include <stdio.h>

#define PROCMGR_NOPRINT (-1)
#define PROCMGR_WARNING 0
#define PROCMGR_INFO    1
#define PROCMGR_DEBUG   2

#define PROCMGR_PRINT_LEVEL PROCMGR_NOPRINT

#define PREFIX "[procmgr]"

#if PROCMGR_PRINT_LEVEL >= PROCMGR_WARNING
#define warn(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define warn(fmt, ...)
#endif

#if PROCMGR_PRINT_LEVEL >= PROCMGR_INFO
#define info(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)
#else
#define info(fmt, ...)
#endif

#if PROCMGR_PRINT_LEVEL >= PROCMGR_DEBUG
#define debug(fmt, ...) \
        printf(PREFIX "<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define error(fmt, ...) printf(PREFIX " " fmt, ##__VA_ARGS__)

#endif /* PROCMGR_DBG_H */