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
#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "perf.h"
#include "pmu.h"

#define cycle_t uint64_t

typedef struct {
        struct timeval time;
        cycle_t cycle;
        unsigned long long time_sum;
        cycle_t cycle_sum;
} perf_time_t;

int bind_thread_to_core(int core_num);

/* Print result for perf tests */
void print_perf_result(perf_time_t *start_perf_time, perf_time_t *end_perf_time,
                       unsigned long count);

void record_time(perf_time_t *perf_time);

/* Accumulate one record */
void accumulate_perf_record(perf_time_t *start_perf_time,
                            perf_time_t *end_perf_time,
                            perf_time_t *total_perf_time);

/* Print result for accumulated perf tests */
void print_accumulate_perf_result(perf_time_t *total_perf_time,
                                  unsigned long count);