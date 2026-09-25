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

#ifndef CHCORE_DEBUG_LOCK
#define CHCORE_DEBUG_LOCK

static inline void chcore_spin_lock(volatile int *lk)
{
	for (;;) {
		if (!__atomic_test_and_set(lk, __ATOMIC_ACQUIRE))
			break;
		while (*lk)
			__asm__ __volatile__("nop" ::: "memory");
			/* TODO Set HCR_EL2.{TWE, TWI} to 0 to allow enter low power mode in user space */
			// __asm__ __volatile__("wfe" ::: "memory");
	}
}

static inline void chcore_spin_unlock(volatile int *lk)
{
	__asm__ __volatile__("dmb ish" ::: "memory");
	*lk = 0;
	/* TODO Set HCR_EL2.{TWE, TWI} to 0 to allow enter low power mode in user space */
	// __asm__ __volatile__("sev" ::: "memory");
}

#endif
