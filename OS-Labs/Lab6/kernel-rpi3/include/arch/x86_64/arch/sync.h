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

#include <common/types.h>

#define mb() asm volatile("mfence")
#define rmb() asm volatile("mfence")
#define wmb() asm volatile("mfence")

#define smp_mb() asm volatile("mfence")
#define smp_rmb() asm volatile("mfence")
#define smp_wmb() asm volatile("mfence")

#define CPU_PAUSE() asm volatile("pause\n":::"memory")
#define COMPILER_BARRIER() asm volatile("":::"memory")

// clang-format off
static inline s32 compare_and_swap_32(s32 * ptr, s32 oldval, s32 newval)
{
        __asm__ __volatile__("lock cmpxchg %[newval], %[ptr]":"+a"(oldval),
                        [ptr] "+m"(*ptr)
                        :[newval] "r"(newval)
                        :"memory");
        return oldval;
}

static inline s64 compare_and_swap_64(s64 * ptr, s64 oldval, s64 newval)
{
        __asm__ __volatile__("lock cmpxchgq %[newval], %[ptr]":"+a"(oldval),
                        [ptr] "+m"(*ptr)
                        :[newval] "r"(newval)
                        :"memory");
        return oldval;
}
// clang-format on

#define atomic_bool_compare_exchange_64(ptr, compare, exchange) \
        ((compare) == compare_and_swap_64((ptr), (compare), (exchange))? 1:0)
#define atomic_bool_compare_exchange_32(ptr, compare, exchange) \
        ((compare) == compare_and_swap_32((ptr), (compare), (exchange))? 1:0)

#define atomic_cmpxchg_32 compare_and_swap_32
#define atomic_cmpxchg_64 compare_and_swap_64

// clang-format off
#define __atomic_xadd(ptr, val, len, width)			\
        ({								\
         u##len oldval = val;					\
         asm volatile ("lock xadd" #width " %0, %1"		\
                         : "+r" (oldval), "+m" (*(ptr))		\
                         : : "memory", "cc");			\
                         oldval;							\
                         })
// clang-format on

#define atomic_fetch_sub_32(ptr, val) __atomic_xadd(ptr, -val, 32, l)
#define atomic_fetch_sub_64(ptr, val) __atomic_xadd(ptr, -val, 64, q)
#define atomic_fetch_add_32(ptr, val) __atomic_xadd(ptr, val, 32, l)
#define atomic_fetch_add_64(ptr, val) __atomic_xadd(ptr, val, 64, q)

// clang-format off
static inline s64 atomic_exchange_64(void *ptr, s64 x)
{
        __asm__ __volatile__("xchgq %0,%1":"=r"((s64)x)
                        :"m"(*(s64 *)ptr),
                        "0"((s64)x)
                        :"memory");

        return x;
}
// clang-format on
