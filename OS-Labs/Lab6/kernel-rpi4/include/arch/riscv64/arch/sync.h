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

/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

/* Reference: riscv_atomic.c and riscv_barrier.h from OpenSBI. */

#pragma once

#include <common/types.h>

/* RISC-V Barriers. */
#define RISCV_FENCE(p, s) \
        asm volatile ("fence " #p "," #s : : : "memory")

#define mb()            RISCV_FENCE(iorw, iorw)
#define rmb             RISCV_FENCE(ir, ir)
#define wmb()           RISCV_FENCE(ow, ow)

#define smp_mb()        RISCV_FENCE(rw, rw)
#define smp_rmb()       RISCV_FENCE(r, r)
#define smp_wmb()       RISCV_FENCE(w, w)

#define COMPILER_BARRIER() asm volatile("":::"memory")

// clang-format off
/* Atomic CAS instructions. */
#define __cmpxchg(ptr, old, new, size)                                    \
	({                                                                \
		__typeof__(ptr) __ptr	 = (ptr);                         \
		__typeof__(*(ptr)) __old = (old);                         \
		__typeof__(*(ptr)) __new = (new);                         \
		__typeof__(*(ptr)) __ret;                                 \
		register unsigned int __rc;                               \
		switch (size) {                                           \
		case 4:                                                   \
			__asm__ __volatile__("0:	lr.w %0, %2\n"    \
					     "	bne  %0, %z3, 1f\n"       \
					     "	sc.w.rl %1, %z4, %2\n"    \
					     "	bnez %1, 0b\n"            \
					     "	fence rw, rw\n"           \
					     "1:\n"                       \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__old), "rJ"(__new)   \
					     : "memory");                 \
			break;                                            \
		case 8:                                                   \
			__asm__ __volatile__("0:	lr.d %0, %2\n"    \
					     "	bne %0, %z3, 1f\n"        \
					     "	sc.d.rl %1, %z4, %2\n"    \
					     "	bnez %1, 0b\n"            \
					     "	fence rw, rw\n"           \
					     "1:\n"                       \
					     : "=&r"(__ret), "=&r"(__rc), \
					       "+A"(*__ptr)               \
					     : "rJ"(__old), "rJ"(__new)   \
					     : "memory");                 \
			break;                                            \
		default:                                                  \
			break;                                            \
		}                                                         \
		__ret;                                                    \
	})
// clang-format on

#define cmpxchg(ptr, o, n)                                          \
	({                                                          \
		__typeof__(*(ptr)) _o_ = (o);                       \
		__typeof__(*(ptr)) _n_ = (n);                       \
		(__typeof__(*(ptr)))                                \
			__cmpxchg((ptr), _o_, _n_, sizeof(*(ptr))); \
	})

static inline s32 atomic_cmpxchg_32(s32 *ptr, s32 oldval, s32 newval)
{
#ifdef __riscv_atomic
	return __sync_val_compare_and_swap(ptr, oldval, newval);
#else
	return cmpxchg(ptr, oldval, newval);
#endif
}

static inline s64 atomic_cmpxchg_64(s64 *ptr, s64 oldval, s64 newval)
{
#ifdef __riscv_atomic
	return __sync_val_compare_and_swap(ptr, oldval, newval);
#else
	return cmpxchg(ptr, oldval, newval);
#endif
}

/* 
 * Atomic Fetch and xxx instructions.
 * Directly use GCC built-in functions.
 */
#ifdef __riscv_atomic
#define atomic_fetch_add_32(ptr, val) __sync_fetch_and_add(ptr, val)
#define atomic_fetch_add_64(ptr, val) __sync_fetch_and_add(ptr, val)
#define atomic_fetch_sub_32(ptr, val) __sync_fetch_and_sub(ptr, val)
#define atomic_fetch_sub_64(ptr, val) __sync_fetch_and_sub(ptr, val)
#else
#define atomic_fetch_add_32(ptr, val) BUG("atomic_fetch_and_add not supported.")
#define atomic_fetch_add_64(ptr, val) BUG("atomic_fetch_and_add not supported.")
#define atomic_fetch_sub_32(ptr, val) BUG("atomic_fetch_and_sub not supported.")
#define atomic_fetch_sub_64(ptr, val) BUG("atomic_fetch_and_sub not supported.")
#endif
