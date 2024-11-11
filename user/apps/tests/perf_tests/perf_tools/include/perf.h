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

#define PREFIX  "[perf]"
#define EPREFIX "[error]"

#define info(fmt, ...)  printf(PREFIX " " fmt, ##__VA_ARGS__)
#define error(fmt, ...) printf(EPREFIX " " fmt, ##__VA_ARGS__)

#define PRIO 255
#if defined(__sparc__)
#define TICK_TO_NANO   1000
#define PERF_NUM_LARGE 10000
#define PERF_NUM_SMALL 100
#define PERF_BY_TIME
#else
#define PERF_NUM_LARGE 1000000
#define PERF_NUM_SMALL 128
#define FS_TEST_LARGE_FILE
#endif