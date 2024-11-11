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

#include <common/types.h>
#include <common/errno.h>
#include <common/macro.h>
#include <common/lock.h>
#include <common/kprint.h>
#include <arch/sync.h>

/* Simple/compact RWLock from linux kernel */

int rwlock_init(struct rwlock *rwlock)
{
	if (rwlock == 0)
		return -EINVAL;
	rwlock->lock = 0;
	return 0;
}

// clang-format off
/* WARN: when there are more than 0x7FFFFFFF readers exist, this function
 * will not function correctly */
void read_lock(struct rwlock *rwlock)
{
        unsigned int tmp, tmp2;
        asm volatile(
                        "1:	ldaxr	%w0, %2\n"
                        "	add	%w0, %w0, #1\n"
                        "	tbnz	%w0, #31, 1b\n"
                        "	stxr	%w1, %w0, %2\n"
                        "	cbnz	%w1, 1b\n"
                        : "=&r" (tmp), "=&r" (tmp2), "+Q" (rwlock->lock)
                        :
                        : "cc", "memory");
}

int read_try_lock(struct rwlock *rwlock)
{
        unsigned int tmp, tmp2;

        asm volatile(
                        /* w1 = 1: stxr failed */
                        "	mov	%w1, #1\n"
                        "1:	ldaxr	%w0, %2\n"
                        "	add	%w0, %w0, #1\n"
                        /* 31 bit has been set => writer */
                        "	tbnz	%w0, #31, 2f\n"
                        "	stxr	%w1, %w0, %2\n"
                        "2:"
                        : "=&r" (tmp), "=&r" (tmp2), "+Q" (rwlock->lock)
                        :
                        : "cc", "memory");

        /* Fail: tmp2 > 0 (stxr fail tmp2 = 1) return -1 */
        return tmp2? -1: 0;
}

void read_unlock(struct rwlock *rwlock)
{
        unsigned int tmp, tmp2;

        asm volatile(
                        "1:	ldxr	%w0, %2\n"
                        "	sub	%w0, %w0, #1\n"
                        "	stlxr	%w1, %w0, %2\n"
                        "	cbnz	%w1, 1b"
                        : "=&r" (tmp), "=&r" (tmp2), "+Q" (rwlock->lock)
                        :
                        : "memory");
}


/* Writer lock, use the 31 bit of rwlock->lock (0x80000000) */

void write_lock(struct rwlock *rwlock)
{
        unsigned int tmp;

        asm volatile(
                        "1:	ldaxr	%w0, %1\n"
                        "	cbnz	%w0, 1b\n"
                        "	stxr	%w0, %w2, %1\n"
                        "	cbnz	%w0, 1b\n"
                        : "=&r" (tmp), "+Q" (rwlock->lock)
                        : "r" (0x80000000)
                        : "memory");
}

/* Writer trylock, use the 31 bit of rwlock->lock (0x80000000) */

int write_try_lock(struct rwlock *rwlock)
{
        unsigned int tmp;

        asm volatile(
                        "1:	ldaxr	%w0, %1\n"
                        "	cbnz	%w0, 2f\n"
                        "	stxr	%w0, %w2, %1\n"
                        "2:"
                        : "=&r" (tmp), "+Q" (rwlock->lock)
                        : "r" (0x80000000)
                        : "memory");

        /* Fail: tmp > 0 (stxr fail tmp = 1) return -1 */
        return tmp? -1: 0;
}


/* Writer unlock, set the rwlock to zero */

void write_unlock(struct rwlock *rwlock)
{
        asm volatile(
                        "stlr	wzr, %0"
                        : "=Q" (rwlock->lock) :: "memory");
}

// clang-format off
