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

#ifndef ARCH_AARCH64_ARCH_SYNC_H
#define ARCH_AARCH64_ARCH_SYNC_H

#include <common/types.h>

#define COMPILER_BARRIER() asm volatile("":::"memory")

#define sev()		asm volatile("sev" : : : "memory")
#define wfe()		asm volatile("wfe" : : : "memory")
#define wfi()		asm volatile("wfi" : : : "memory")

#define isb()		asm volatile("isb" : : : "memory")
#define dmb(opt)	asm volatile("dmb " #opt : : : "memory")
#define dsb(opt)	asm volatile("dsb " #opt : : : "memory")

#define mb()		dsb(sy)
#define rmb()		dsb(ld)
#define wmb()		dsb(st)

#define smp_mb()	dmb(ish)
#define smp_rmb()	dmb(ishld)
#define smp_wmb()	dmb(ishst)

#define dma_rmb()	dmb(oshld)
#define dma_wmb()	dmb(oshst)

#define ldar_32(ptr, value) asm volatile("ldar %w0, [%1]" : "=r" (value) : "r" (ptr))
#define stlr_32(ptr, value) asm volatile("stlr %w0, [%1]" : : "rZ" (value) , "r" (ptr))

#define ldar_64(ptr, value) asm volatile("ldar %x0, [%1]" : "=r" (value) : "r" (ptr))
#define stlr_64(ptr, value) asm volatile("stlr %x0, [%1]" : : "rZ" (value) , "r" (ptr))

// clang-format off
#define __atomic_compare_exchange(ptr, compare, exchange, len, width)	\
({									\
        u##len oldval;							\
        u32 ret;							\
        asm volatile (  "1: ldaxr   %"#width"0, %2\n"			\
                        "   cmp     %"#width"0, %"#width"3\n"		\
                        "   b.ne    2f\n"				\
                        "   stlxr   %w1, %"#width"4, %2\n"		\
                        "   cbnz    %w1, 1b\n"				\
                        "2:":"=&r" (oldval), "=&r"(ret), "+Q"(*(ptr))	\
                        :"r"(compare), "r"(exchange)			\
                        );						\
        oldval;								\
})
// clang-format on

#define atomic_compare_exchange_64(ptr, compare, exchange) \
	__atomic_compare_exchange(ptr, compare, exchange, 64, x)
#define atomic_compare_exchange_32(ptr, compare, exchange) \
	__atomic_compare_exchange(ptr, compare, exchange, 32, w)

#define atomic_cmpxchg_32 atomic_compare_exchange_32
#define atomic_cmpxchg_64 atomic_compare_exchange_64

// clang-format off
static inline s64 atomic_exchange_64(s64 * ptr, s64 exchange)
{
        s64 oldval;
        s32 ret;
        asm volatile (  "1: ldaxr   %x0, %2\n"
                        "   stlxr   %w1, %x3, %2\n"
                        "   cbnz    %w1, 1b\n"
                        "2:":"=&r" (oldval), "=&r"(ret), "+Q"(*ptr)
                        :"r"(exchange)
                     );
        return oldval;
}

#define __atomic_fetch_op(ptr, val, len, width, op)			\
({									\
        u##len oldval, newval;						\
        u32 ret;							\
         asm volatile ( "1: ldaxr   %"#width"0, %3\n"			\
                        "   "#op"   %"#width"1, %"#width"0, %"#width"4\n"\
                        "   stlxr   %w2, %"#width"1, %3\n"		\
                        "   cbnz    %w2, 1b\n"				\
                        "2:":"=&r" (oldval), "=&r"(newval),		\
                        "=&r"(ret), "+Q"(*(ptr))			\
                        :"r"(val)					\
                      );						\
        oldval;								\
})
// clang-format on

#define atomic_fetch_sub_32(ptr, val) __atomic_fetch_op(ptr, val, 32, w, sub)
#define atomic_fetch_sub_64(ptr, val) __atomic_fetch_op(ptr, val, 64, x, sub)
#define atomic_fetch_add_32(ptr, val) __atomic_fetch_op(ptr, val, 32, w, add)
#define atomic_fetch_add_64(ptr, val) __atomic_fetch_op(ptr, val, 64, x, add)
#define atomic_set_bit_32(ptr, val)   __atomic_fetch_op(ptr, 1<<(val), 32, w, or)

static inline u64 atomic_fetch_add_64_unless(u64 * ptr, int add, int not_expect)
{
	u64 val = *ptr, oldval;
	smp_rmb();

	while (true) {
		if (val == not_expect)
			break;
		oldval = atomic_compare_exchange_64(ptr, val, val + add);
		if (oldval == val)
			break;
		val = *ptr;
	}

	return val;
}

#endif /* ARCH_AARCH64_ARCH_SYNC_H */
