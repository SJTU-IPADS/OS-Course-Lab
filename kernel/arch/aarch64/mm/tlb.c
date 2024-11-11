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

#include <arch/mm/page_table.h>
#include <common/types.h>
#include <common/macro.h>
#include <mm/vmspace.h>
#include <mm/mm.h>
#include <arch/sync.h>

/*
 * Invalidate TLB template:
 *  DSB: ensure page table updates are finished
 *  TLBI
 *  DSB: ensure TLB invalidation is finished
 *  ISB: ensure the instruction fetching is using new mappings
 */
#define TLBI_ASID_SHIFT 48

/* Flush tlbs in all the cores by asid. */
static void flush_tlb_by_asid(u64 asid)
{
	/* inner sharable barrier */
	dsb(ish);
	asm volatile("tlbi aside1is, %0\n"
		     :
		     : "r" (asid << TLBI_ASID_SHIFT)
		     : );
	dsb(ish);
	isb();
}

/* Flush tlbs of designated VAs. The asid info is encoded in @addr_arg. */
static void flush_tlb_addr_asid(u64 addr_arg, u64 page_cnt)
{
	u64 i;

	/* inner sharable barrier */
	dsb(ish);
	for (i = 0; i < page_cnt; ++i) {
		asm volatile("tlbi vae1is, %0\n"
		     :
		     : "r" (addr_arg)
		     : );
		addr_arg++;
	}
	dsb(ish);
	isb();
}

/*
 * The arg for 'tlbi vae1is': | ASID | TTL | VA (virtual frame number) |.
 * If ARMv8.4-TTL is not supported, TTL should be 0.
 */
static u64 get_tlbi_va_arg(vaddr_t addr, u64 asid)
{
	vaddr_t arg;

	arg = addr >> PAGE_SHIFT;
	arg |= asid << TLBI_ASID_SHIFT;

	return arg;
}

/* The threshold can be tuned. */
#define TLB_SHOOTDOWN_THRESHOLD 2

static void do_flush_tlb_opt(vaddr_t start_va, u64 page_cnt, u64 asid)
{
	/* flush tlbs in all the cpus */
	if (page_cnt > TLB_SHOOTDOWN_THRESHOLD) {
		/* Flush all the TLBs of the ASID in all the cpus */
		flush_tlb_by_asid(asid);
	} else {
		/* Flush each TLB entry one-by-one in all the cpus */
		flush_tlb_addr_asid(get_tlbi_va_arg(start_va, asid), page_cnt);
	}
}



/* Exposed functions */
void flush_tlb_opt(struct vmspace* vmspace, vaddr_t start_va, size_t len)
{
	u64 page_cnt;
	u64 asid;

	if (unlikely(len < PAGE_SIZE))
		kwarn("func: %s. len (%p) < PAGE_SIZE\n", __func__, len);

	if (len == 0)
		return;

	start_va = ROUND_DOWN(start_va, PAGE_SIZE);
	len = ROUND_UP(len, PAGE_SIZE);
	page_cnt = len / PAGE_SIZE;

	asid = vmspace->pcid;

	do_flush_tlb_opt(start_va, page_cnt, asid);
}

void flush_tlb_by_range(struct vmspace* vmspace, vaddr_t start_va, size_t len)
{
	flush_tlb_opt(vmspace, start_va, len);
}

void flush_tlb_all(void)
{
	/* full system barrier */
	dsb(sy);
	asm volatile("tlbi vmalle1is\n\t" : : :);
	dsb(sy);
	isb();
}

void flush_tlb_by_vmspace(struct vmspace *vmspace)
{
	flush_tlb_by_asid(vmspace->pcid);
}
