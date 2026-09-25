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

// clang-format off
#define _lock_with_swap(lock_ptr, old_val_ptr) \
	asm volatile( \
		"1:	ld [%0], %%l0\n"\
		"	cmp %%l0, -1\n"\
		"	be 1b\n"\
		"	mov -1, %%l0\n"\
		"	swap [%0], %%l0\n"\
		"	cmp %%l0, -1\n"\
		"	be 1b\n"\
		"	st %%l0, [%1]\n"\
		:\
		: "r" (lock_ptr), "r" (old_val_ptr)\
		: "%l0")

#define _unlock_with_swap(lock_ptr, new_val_ptr) \
	asm volatile(\
		"ld [%0], %%l0\n"\
		"stbar\n"\
		"st %%l0, [%1]\n"\
		:\
		: "r" (new_val_ptr), "r" (lock_ptr)\
		: "%l0")
// clang-format on

#ifdef PLAT_HAVE_CASA
static inline unsigned long compare_and_swap_32(unsigned long * ptr, unsigned long oldval, unsigned long newval)
{
	asm volatile(
		"casa [%1] 0xb, %2, %0\n"
		: "+r" (newval) : "r" (ptr), "r" (oldval) : "memory"
	);
	return newval;
}
#else
static inline unsigned long compare_and_swap_32(unsigned long * ptr, unsigned long oldval, unsigned long newval)
{
	/* XXX: Trap and irq must not occur */
	if (*ptr == oldval) {
		*ptr = newval;
		return oldval;
	} else {
		return *ptr;
	}
}
#endif

static inline unsigned long fetch_and_add_32(unsigned long *ptr, unsigned long inc)
{
	unsigned long old;

	do {
		old = *ptr;
	} while (compare_and_swap_32(ptr, old, old + inc) != old);
	return old;
}

static inline unsigned long atomic_op_not_impl(void)
{
	BUG("atomic operation not supported.");
	return 0;
}

#define smp_mb() asm volatile("stbar\n")

#define atomic_fetch_add_32(ptr, val) fetch_and_add_32((unsigned long *)ptr, val)
#define atomic_fetch_add_64(ptr, val) atomic_op_not_impl()
#define atomic_fetch_sub_32(ptr, val) fetch_and_add_32((unsigned long *)ptr, -val)
#define atomic_fetch_sub_64(ptr, val) atomic_op_not_impl()

#define atomic_cmpxchg_32(ptr, cmp, exchg) \
	compare_and_swap_32((unsigned long *)ptr, cmp, exchg)
