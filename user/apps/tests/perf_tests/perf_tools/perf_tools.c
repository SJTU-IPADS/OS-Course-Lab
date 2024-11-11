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

#include "perf_tools.h"
#include <inttypes.h>

int bind_thread_to_core(int core_num)
{
        int ret;
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(core_num, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        if (ret < 0)
                return ret;
        sched_yield();
        return ret;
}

void print_perf_result(perf_time_t *start_perf_time, perf_time_t *end_perf_time,
                       unsigned long count)
{
#if defined(PERF_BY_TIME)
        unsigned long long total_time;
        unsigned long long single_time;

        total_time =
                (end_perf_time->time.tv_sec - start_perf_time->time.tv_sec)
                        * 1000000
                + (end_perf_time->time.tv_usec - start_perf_time->time.tv_usec);
        single_time = total_time * 1.0 / count;

        // printf("Total Time cost: %ld us\n", total_time);
        printf("Single Time cost (nearly): %lld us\n", single_time);
        cycle_t cycle;
        cycle = single_time * 1000 / TICK_TO_NANO;
        printf("Single Cycle cost (nearly): %lld tick\n", cycle);
#else
        cycle_t total_cycle = end_perf_time->cycle - start_perf_time->cycle;
        // printf("Total Cycle cost: %" PRId64 "cycles\n", total_cycle);
        printf("Single Cycle cost (nearly): %" PRId64 " cycles\n",
               total_cycle / count);
#endif
}

void record_time(perf_time_t *perf_time)
{
#if defined(PERF_BY_TIME)
        gettimeofday(&(perf_time->time), NULL);
#else
        perf_time->cycle = pmu_read_real_cycle();
#endif
}

void accumulate_perf_record(perf_time_t *start_perf_time,
                            perf_time_t *end_perf_time,
                            perf_time_t *total_perf_time)
{
#if defined(PERF_BY_TIME)
        unsigned long long diff_time;

        diff_time =
                (end_perf_time->time.tv_sec - start_perf_time->time.tv_sec)
                        * 1000000
                + (end_perf_time->time.tv_usec - start_perf_time->time.tv_usec);

        total_perf_time->time_sum += diff_time;
#else
        cycle_t diff_cycle = end_perf_time->cycle - start_perf_time->cycle;
        total_perf_time->cycle_sum += diff_cycle;
#endif
}

void print_accumulate_perf_result(perf_time_t *total_perf_time,
                                  unsigned long count)
{
#if defined(PERF_BY_TIME)
        // printf("Total Time cost: %ld us\n", total_perf_time->time_sum);
        printf("Single Time cost (nearly): %lld us\n",
               total_perf_time->time_sum / count);
        cycle_t cycle;
        cycle = (total_perf_time->time_sum / count) * 1000 / TICK_TO_NANO;
        printf("Single Cycle cost (nearly): %lld tick\n", cycle);
#else
        // printf("Total Cycle cost: %" PRId64 " cycles\n",
        // total_perf_time->cycle_sum);
        printf("Single Cycle cost (nearly): %" PRId64 " cycles\n",
               total_perf_time->cycle_sum / count);
#endif
}
