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

#include <arch/machine/registers.h>
#include <arch/machine/smp.h>
#include <mm/cache.h>
#include <common/types.h>

/*
 * DCZID_EL0: Data Cache Zero ID register
 * DZP, bit [4] used to indicate whether use of DC ZVA instructions is permitted or not
 * BS, bit [3:0] used to indicate log2 of the block size in words.
 * The maximum size supported is 2KB (value == 9).
 */

/* A global varible to inidicate block size which will be cleaned by dc zva call */
int dczva_line_size = 0;

#define DZP_SHIFT 4
#define DZBS_MASK 0xf

/*
 * Read Data Cache Zero ID register
 */
long read_dczid(void)
{
	long val;

	asm volatile("mrs %0, dczid_el0\n\t" : "=r"(val));
	return val;
}

/*
 * Check whether support DC ZVA and get dczva_line_size
 */
void cache_setup(void)
{
	long dczid_val;
	int dczid_bs;

	dczid_val = read_dczid();
	if (dczid_val & (1 << DZP_SHIFT)) {
		/* the 4th bit indicates the instruction is disabled */
		dczva_line_size = 0;
	} else {
		/* the zero size stores in the last four bits */
		dczid_bs = dczid_val & (DZBS_MASK);
		dczva_line_size = sizeof(int) << dczid_bs;
	}
}

#define CACHE_LINE_LENGTH 64

#define ICACHE_POLICY_VPIPT	0
#define ICACHE_POLICY_AIVIVT	1
#define ICACHE_POLICY_VIPT	2
#define ICACHE_POLICY_PIPT	3

static inline void __dcache_clean_area(u64 start, u64 end)
{
	while (start < end) {
		asm volatile("dc cvac, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_LENGTH;
	}
}

static inline void __dcache_clean_area_pou(u64 start, u64 end)
{
	while (start < end) {
		asm volatile("dc cvau, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_LENGTH;
	}
}

static inline void __dcache_inv_area(u64 start, u64 end)
{
	while (start < end) {
		asm volatile("dc ivac, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_LENGTH;
	}
}

static inline void __dcache_clean_and_inv_area(u64 start, u64 end)
{
	while (start < end) {
		asm volatile("dc civac, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_LENGTH;
	}
}

static inline void flush_icache_all(void)
{
	/*
	 * ic iallu is not enough. We need instructions to be data
	 * coherence in the inner shareable domain. So we use ic ialluis:
	 * Invalidate instruction cache ALL to PoU (Inner Shareable)
	 */
	asm volatile("ic ialluis");
}

static inline void flush_icache_range(u64 start, u64 end)
{
	while (start < end) {
		asm volatile("ic ivau, %0" : : "r"(start) : "memory");
		start += CACHE_LINE_LENGTH;
	}
}

static inline bool is_icache_aliasing(u64 ctr_el0)
{
	/*
	 * VIVT icache has cache cache alias issue.
	 * VIPT icache is either aliasing or non-aliasing
	 * depending on the highest index bit and PAGE_SHIFT.
	 * We just assume all VIPT icache to be aliasing.
	 */
	switch ((ctr_el0  >> CTR_EL0_L1Ip_SHIFT) & CTR_EL0_L1Ip_MASK) {
	case ICACHE_POLICY_PIPT:
	case ICACHE_POLICY_VPIPT:
		return false;
	case ICACHE_POLICY_AIVIVT:
	case ICACHE_POLICY_VIPT:
		/* Assume aliasing */
		return true;
	default:
		BUG("Unknown instruction cache policy!");
		return false;
	}
}

static inline void __sync_idcache(u64 start, u64 end)
{
	/*
	 * If IDC bit is set, clean data cache is not required for
	 * instruction to data coherence.
	 */
	if (ctr_el0 & CTR_EL0_IDC) {
		asm volatile("dsb ishst");
	} else {
		__dcache_clean_area_pou(start, end);
		asm volatile("dsb ish");
	}

	/*
	 * If DIC bit is set, invalidate instruction cache is not required for
	 * data to instruction coherence.
	 */
	if (!(ctr_el0 & CTR_EL0_DIC)) {
		if (is_icache_aliasing(ctr_el0)) {
			flush_icache_all();
		} else {
			flush_icache_range(start, end);
		}
	}
}

void arch_flush_cache(vaddr_t start, size_t len, int op_type)
{
	u64 real_start;
	u64 real_end;

	real_start = ROUND_DOWN(start, CACHE_LINE_LENGTH);
	real_end = ROUND_UP(start + len, CACHE_LINE_LENGTH);
	BUG_ON(real_start > real_end);

	switch (op_type) {
	case CACHE_CLEAN:
		__dcache_clean_area(real_start, real_end);
		break;
	case CACHE_INVALIDATE:
		__dcache_inv_area(real_start, real_end);
		break;
	case CACHE_CLEAN_AND_INV:
		__dcache_clean_and_inv_area(real_start, real_end);
		break;
	case SYNC_IDCACHE:
		__sync_idcache(real_start, real_end);
		break;
	default:
		BUG("Unsupported cache operation type: %d\n", op_type);
		break;
	}
	asm volatile("dsb ish");
	asm volatile("isb");
}


/*
void flush_icache_range(u64 start, u64 size)
{
	__builtin___clear_cache((char*)start, (char*)(start + size));
}
*/
