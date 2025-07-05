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
/* Intel SDM Vol.3A Figure 4-11 */

#define PAGE_BIT_PRESENT	0	/* is present */
#define PAGE_BIT_RW		1	/* writeable */
#define PAGE_BIT_USER		2	/* userspace addressable */
#define PAGE_BIT_PWT		3	/* page write through */
#define PAGE_BIT_PCD		4	/* page cache disabled */
#define PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define PAGE_BIT_PAT		7	/* on 4KB pages */
#define PAGE_BIT_GLOBAL	        8	/* Global TLB entry PPro+ */
#define PAGE_BIT_SOFTW1	        9	/* available for programmer */
#define PAGE_BIT_SOFTW2	        10	/* " */
#define PAGE_BIT_SOFTW3	        11	/* " */
#define PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define PAGE_BIT_SOFTW4	        58	/* available for programmer */
#define PAGE_BIT_PKEY_BIT0	59	/* Protection Keys, bit 1/4 */
#define PAGE_BIT_PKEY_BIT1	60	/* Protection Keys, bit 2/4 */
#define PAGE_BIT_PKEY_BIT2	61	/* Protection Keys, bit 3/4 */
#define PAGE_BIT_PKEY_BIT3	62	/* Protection Keys, bit 4/4 */
#define PAGE_BIT_NX		63	/* No execute: only valid after cpuid check */

#define PAGE_PRESENT	(1UL << PAGE_BIT_PRESENT)
#define PAGE_RW	        (1UL << PAGE_BIT_RW)
#define PAGE_USER	(1UL << PAGE_BIT_USER)
#define PAGE_PWT	(1UL << PAGE_BIT_PWT)
#define PAGE_PCD	(1UL << PAGE_BIT_PCD)
#define PAGE_ACCESSED	(1UL << PAGE_BIT_ACCESSED)
#define PAGE_DIRTY	(1UL << PAGE_BIT_DIRTY)
#define PAGE_PSE	(1UL << PAGE_BIT_PSE)
#define PAGE_GLOBAL	(1UL << PAGE_BIT_GLOBAL)
#define PAGE_SOFTW1	(1UL << PAGE_BIT_SOFTW1)
#define PAGE_SOFTW2	(1UL << PAGE_BIT_SOFTW2)
#define PAGE_PAT	(1UL << PAGE_BIT_PAT)
#define PAGE_PAT_LARGE  (1UL << PAGE_BIT_PAT_LARGE)
#define PAGE_SPECIAL	(1UL << PAGE_BIT_SPECIAL)
#define PAGE_CPA_TEST	(1UL << PAGE_BIT_CPA_TEST)

#define PAGE_PKEY_BIT0	(1UL << PAGE_BIT_PKEY_BIT0)
#define PAGE_PKEY_BIT1	(1UL << PAGE_BIT_PKEY_BIT1)
#define PAGE_PKEY_BIT2	(1UL << PAGE_BIT_PKEY_BIT2)
#define PAGE_PKEY_BIT3	(1UL << PAGE_BIT_PKEY_BIT3)
#define PAGE_NX         (1UL << PAGE_BIT_NX)


#define PAGE_SHIFT                                (12)
#ifndef PAGE_SIZE
#define PAGE_SIZE                                 (1 << (PAGE_SHIFT))
#endif
#define PAGE_MASK                                 (PAGE_SIZE - 1)
#define PAGE_ORDER                                (9)

#define PTP_ENTRIES				  (1 << PAGE_ORDER)
#define L3					  (3)
#define L2					  (2)
#define L1					  (1)
#define L0					  (0)

#define PTP_INDEX_MASK				  ((1 << (PAGE_ORDER)) - 1)
#define L0_INDEX_SHIFT			    ((3 * PAGE_ORDER) + PAGE_SHIFT)
#define L1_INDEX_SHIFT			    ((2 * PAGE_ORDER) + PAGE_SHIFT)
#define L2_INDEX_SHIFT			    ((1 * PAGE_ORDER) + PAGE_SHIFT)
#define L3_INDEX_SHIFT			    ((0 * PAGE_ORDER) + PAGE_SHIFT)

#define GET_L0_INDEX(addr) ((addr >> L0_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L1_INDEX(addr) ((addr >> L1_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L2_INDEX(addr) ((addr >> L2_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L3_INDEX(addr) ((addr >> L3_INDEX_SHIFT) & PTP_INDEX_MASK)


#define x64_MMU_L1_BLOCK_ORDER                  (18)
#define x64_MMU_L2_BLOCK_ORDER                  (9)
#define x64_MMU_L3_PAGE_ORDER                   (0)

#define x64_MMU_L0_BLOCK_PAGES  (PTP_ENTRIES * x64_MMU_L1_BLOCK_PAGES)
#define x64_MMU_L1_BLOCK_PAGES  (1UL << x64_MMU_L1_BLOCK_ORDER)
#define x64_MMU_L2_BLOCK_PAGES  (1UL << x64_MMU_L2_BLOCK_ORDER)
#define x64_MMU_L3_PAGE_PAGES   (1UL << x64_MMU_L3_PAGE_ORDER)

#define L0_PER_ENTRY_PAGES	  (x64_MMU_L0_BLOCK_PAGES)
#define L1_PER_ENTRY_PAGES	  (x64_MMU_L1_BLOCK_PAGES)
#define L2_PER_ENTRY_PAGES        (x64_MMU_L2_BLOCK_PAGES)
#define L3_PER_ENTRY_PAGES	  (x64_MMU_L3_PAGE_PAGES)

#define x64_MMU_L1_BLOCK_MASK     ((1 << L1_INDEX_SHIFT) - 1)
#define x64_MMU_L2_BLOCK_MASK     ((1 << L2_INDEX_SHIFT) - 1)
#define x64_MMU_L3_BLOCK_MASK     ((1 << L3_INDEX_SHIFT) - 1)

#define GET_VA_OFFSET_L1(va)      (va & x64_MMU_L1_BLOCK_MASK)
#define GET_VA_OFFSET_L2(va)      (va & x64_MMU_L2_BLOCK_MASK)
#define GET_VA_OFFSET_L3(va)      (va & x64_MMU_L3_BLOCK_MASK)

// clang-format off
/* table format */
typedef union {
        struct {
                u64 present	    : 1,
                    writeable       : 1,
                    user            : 1,
                    write_through   : 1,
                    cache_disable   : 1,
                    accessed        : 1,
                    ignored1        : 1,  /* no dirty bit */
                    is_page         : 1,  /* must be 0 */
                    ignored2        : 4,  /* ignored */
                    next_table_addr : 36, /* TODO: we assume 48-bit physical address now */
                    reserved2       : 4,  /* bit [48:51] must be 0 */
                    ignored3        : 11,
                    nx              : 1;  /* bit 63: execution disable */
        } table; /* pml4, pdpte, pde */

        struct {
                u64 present	    : 1,
                    writeable       : 1,
                    user            : 1,
                    write_through   : 1,
                    cache_disable   : 1,
                    accessed        : 1,
                    dirty           : 1,
                    is_page         : 1,  /* must be 1 */
                    global          : 1,  /* global TLB entry */
                    ignored1        : 3,  /* ignored */
                    pat             : 1,  /* bit 12 */
                    reserved2       : 17, /* must be 0 */
                    pfn             : 18, /* TODO: we assume 48-bit physical address now */
                    reserved        : 4,  /* bit [48:51] must be 0 */
                    ignored         : 7,
                    pkey            : 4,  /* bit [59:62] pkey domain ID */
                    nx              : 1;  /* bit 63: execution disable */
        } pte_1G;

        struct {
                u64 present	    : 1,
                    writeable       : 1,
                    user            : 1,
                    write_through   : 1,
                    cache_disable   : 1,
                    accessed        : 1,
                    dirty           : 1,
                    is_page         : 1,  /* must be 1 */
                    global          : 1,  /* global TLB entry */
                    ignored1        : 3,  /* ignored */
                    pat             : 1,  /* bit 12 */
                    reserved2       : 8,  /* must be 0 */
                    pfn             : 27, /* TODO: we assume 48-bit physical address now */
                    reserved        : 4,  /* bit [48:51] must be 0 */
                    ignored         : 7,
                    pkey            : 4,  /* bit [59:62] pkey domain ID */
                    nx              : 1;  /* bit 63: execution disable */
        } pte_2M;

        struct {
                u64 present	    : 1,
                    writeable       : 1,
                    user            : 1,
                    write_through   : 1,
                    cache_disable   : 1,
                    accessed        : 1,
                    dirty           : 1,
                    pat             : 1,  /* memory type */
                    global          : 1,  /* global TLB entry */
                    ignored1        : 3,  /* ignored */
                    pfn             : 36, /* TODO: we assume 48-bit physical address now */
                    reserved        : 4,  /* bit [48:51] must be 0 */
                    ignored         : 7,
                    pkey            : 4,  /* bit [59:62] pkey domain ID */
                    nx              : 1;  /* bit 63: execution disable */
        } pte_4K;

        u64 pteval;
} pte_t;
// clang-format on

/* one page table page */
typedef struct {
	pte_t ent[512];
} ptp_t;

#define GET_PADDR_IN_PTE(entry) \
	(((u64)entry->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define PCID_MASK (0xfffUL)
/*
 * When PCID is enabld,
 * we store PCID in the pgtbl field (last 12 bits) of each process.
 * Remove PCID (last 12 bits) in that (virtual) address.
 */
#define remove_pcid(pgtbl) (void *)(((u64)pgtbl) & (~PCID_MASK))
#define get_pcid(pgtbl) (((u64)pgtbl) & (PCID_MASK))

int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va,
		 ptp_t **next_ptp, pte_t **pte, bool alloc, long *rss);
