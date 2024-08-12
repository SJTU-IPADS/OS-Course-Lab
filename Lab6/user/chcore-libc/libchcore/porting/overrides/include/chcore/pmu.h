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

#include <chcore/type.h>

#if defined(CHCORE_ARCH_SPARC)
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CHCORE_ARCH_AARCH64)

static inline u64 pmu_read_real_cycle(void)
{
        s64 tv;
        asm volatile("mrs %0, pmccntr_el0" : "=r"(tv));
        return tv;
}

static inline void pmu_clear_cnt(void)
{
        asm volatile("msr pmccntr_el0, %0" ::"r"(0));
}

#endif

#if defined(CHCORE_ARCH_X86_64)

static inline u64 pmu_read_real_cycle(void)
{
        u32 lo, hi;

        __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
        return (((u64)hi) << 32) | lo;
}

static inline void pmu_clear_cnt(void)
{
        /* Do Nothing */
}

#endif

#if defined(CHCORE_ARCH_RISCV64)

static inline u64 pmu_read_real_cycle(void)
{
        u64 cycle;
        asm volatile("rdcycle %0" : "=r"(cycle));
        return cycle;
}

static inline void pmu_clear_cnt(void)
{
        /* Do Nothing */
}

#endif

#if defined(CHCORE_ARCH_SPARC)

static inline u64 pmu_read_real_cycle(void)
{
        struct timespec cur_time;

        clock_gettime(CLOCK_MONOTONIC, &cur_time);
        return (u64)1000000000 * cur_time.tv_sec + cur_time.tv_nsec;
}

static inline void pmu_clear_cnt(void)
{
        /* Do Nothing */
}
#endif

#ifdef __cplusplus
}
#endif
