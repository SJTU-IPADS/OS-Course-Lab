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
#include <common/vars.h>

#define SATP_SV39 (8UL << 60)
#define SATP_SV48 (9UL << 60)

#define SV48_PTE_PPN0_OFFSET 10
#define SV48_PTE_PPN1_OFFSET 19
#define SV48_PTE_PPN2_OFFSET 28
#define SV48_PTE_PPN3_OFFSET 37

#define PTE_PPN0_OFFSET 10
#define PTE_PPN1_OFFSET 19
#define PTE_PPN2_OFFSET 28
#define PTE_PPN3_OFFSET 37

#ifdef SV48
#define MAKE_SATP(pagetable) (SATP_SV48 | ((u64)pagetable))
#elif defined(SV39)
#define MAKE_SATP(pagetable) (SATP_SV39 | ((u64)pagetable))
#else
#error "SV32 not supported."
#endif

#define PAGE_SHIFT  12  // bits of offset within a page
#define ASID_SHIFT  44
#define PAGE_ORDER  9
#define PAGE_MASK   (4096 - 1)
#define PTP_ENTRIES (1 << PAGE_ORDER)

#define PTP_INDEX_MASK     ((1 << PAGE_ORDER) - 1)
#define L0_INDEX_SHIFT     ((3 * PAGE_ORDER) + PAGE_SHIFT)
#define L1_INDEX_SHIFT     ((2 * PAGE_ORDER) + PAGE_SHIFT)
#define L2_INDEX_SHIFT     ((1 * PAGE_ORDER) + PAGE_SHIFT)
#define L3_INDEX_SHIFT     ((0 * PAGE_ORDER) + PAGE_SHIFT)
#define GET_L0_INDEX(addr) (((addr) >> L0_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L1_INDEX(addr) (((addr) >> L1_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L2_INDEX(addr) (((addr) >> L2_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L3_INDEX(addr) (((addr) >> L3_INDEX_SHIFT) & PTP_INDEX_MASK)

#define PAGE_1G_BITS  	30
#define PAGE_2M_BITS  	21
#define PAGE_4K_BITS  	12
#define PAGE1G_MASK 	((1 << PAGE_1G_BITS) - 1)

#if defined(SV48)
#define L3_PER_ENTRY_PAGES (1)
#define L2_PER_ENTRY_PAGES (1ul << (1 * PAGE_ORDER))
#define L1_PER_ENTRY_PAGES (PTP_ENTRIES * L2_PER_ENTRY_PAGES)
#define L0_PER_ENTRY_PAGES (PTP_ENTRIES * L1_PER_ENTRY_PAGES)
#elif defined(SV39)
#define L2_PER_ENTRY_PAGES (1)
#define L1_PER_ENTRY_PAGES (1 << (1*PAGE_ORDER))
#define L0_PER_ENTRY_PAGES (PTP_ENTRIES * L1_PER_ENTRY_PAGES)
#else
#error "SV32 not supported."
#endif

#define PTE_VALUE_SHIFT	44
#define PTE_VALUE_MASK  ((1UL << PTE_VALUE_SHIFT) - 1)

/* PTE attributes. */
#define PTE_V   (1L << 0)
#define PTE_R   (1L << 1)
#define PTE_W   (1L << 2)
#define PTE_X   (1L << 3)
#define PTE_U   (1L << 4)
#define PTE_G   (1L << 5)
#define PTE_A   (1L << 6)
#define PTE_D   (1L << 7)

/* Get PA from PTE.PPN. */
#define PTE2PA4K(pte) (((pte) >> 10) << PAGE_4K_BITS)
#define PTE2PA1G(pte) (((pte) >> 28) << PAGE_1G_BITS)

// clang-format off
/* Table format */
typedef union {
        struct {
                u64 valid : 1,
                    read : 1,
                    write : 1,
                    exec : 1,
                    user : 1,
                    global : 1,
                    access : 1,
                    dirty : 1,
                    rsw :  2,
                    ppn0 : 9,
                    ppn1 : 9,
                    ppn2 : 26,
                    reserved : 10;
        } pte;

        struct {
                u64 valid : 1,
                    read : 1,
                    write : 1,
                    exec : 1,
                    user : 1,
                    global : 1,
                    access : 1,
                    dirty : 1,
                    rsw : 2,
                    ppn : 44,
                    reserved : 10;
        } pte_with_merged_ppn;

        u64 pte_val;
} pte_t;
// clang-format on

/* page_table_page type */
typedef struct {
    	pte_t ent[512];
} ptp_t;



