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

#ifndef ARCH_AARCH64_ARCH_MM_PAGE_TABLE_H
#define ARCH_AARCH64_ARCH_MM_PAGE_TABLE_H

#include <common/types.h>

#define INNER_SHAREABLE  (0x3)
/* Please search mair_el1 for these memory types. */
#define NORMAL_MEMORY         (4)
#define NORMAL_MEMORY_NOCACHE (3)
#define DEVICE_MEMORY         (0)

/* Description bits in page table entries. */

/* Read-write permission. */
#define AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW     (1)
#define AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO     (3)

/* X: execution permission. U: unprivileged. P: privileged. */
#define AARCH64_MMU_ATTR_PAGE_UX                    (0)
#define AARCH64_MMU_ATTR_PAGE_UXN                   (1)
#define AARCH64_MMU_ATTR_PAGE_PXN                   (1)

/* Access flag bit. */
#define AARCH64_MMU_ATTR_PAGE_AF_ACCESSED           (1)

/* Present (valid) bit. */
#define AARCH64_MMU_PTE_INVALID_MASK                (1 << 0)
/* Table bit: whether the next level is aonther pte or physical memory page. */
#define AARCH64_MMU_PTE_TABLE_MASK                  (1 << 1)
#define IS_PTE_INVALID(pte) (!((pte) & AARCH64_MMU_PTE_INVALID_MASK))
#define IS_PTE_TABLE(pte) (!!((pte) & AARCH64_MMU_PTE_TABLE_MASK))

/* PAGE_SIZE (4k) == (1 << (PAGE_SHIFT)) */
#define PAGE_SHIFT                          (12)
#define PAGE_MASK                           (PAGE_SIZE - 1)
#define PAGE_ORDER                          (9)

#define PTP_INDEX_MASK			    ((1 << (PAGE_ORDER)) - 1)
#define L0_INDEX_SHIFT			    ((3 * PAGE_ORDER) + PAGE_SHIFT)
#define L1_INDEX_SHIFT			    ((2 * PAGE_ORDER) + PAGE_SHIFT)
#define L2_INDEX_SHIFT			    ((1 * PAGE_ORDER) + PAGE_SHIFT)
#define L3_INDEX_SHIFT			    ((0 * PAGE_ORDER) + PAGE_SHIFT)

#define GET_L0_INDEX(addr) (((addr) >> L0_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L1_INDEX(addr) (((addr) >> L1_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L2_INDEX(addr) (((addr) >> L2_INDEX_SHIFT) & PTP_INDEX_MASK)
#define GET_L3_INDEX(addr) (((addr) >> L3_INDEX_SHIFT) & PTP_INDEX_MASK)

#define PTP_ENTRIES               (1UL << PAGE_ORDER)
/* Number of 4KB-pages that an Lx-block describes */
#define L0_PER_ENTRY_PAGES	  ((PTP_ENTRIES) * (L1_PER_ENTRY_PAGES))
#define L1_PER_ENTRY_PAGES	  ((PTP_ENTRIES) * (L2_PER_ENTRY_PAGES))
#define L2_PER_ENTRY_PAGES	  ((PTP_ENTRIES) * (L3_PER_ENTRY_PAGES))
#define L3_PER_ENTRY_PAGES	  (1)

/* Bitmask used by GET_VA_OFFSET_Lx */
#define L1_BLOCK_MASK   ((L1_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)
#define L2_BLOCK_MASK   ((L2_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)
#define L3_PAGE_MASK    ((L3_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)

#define GET_VA_OFFSET_L1(va)      ((va) & L1_BLOCK_MASK)
#define GET_VA_OFFSET_L2(va)      ((va) & L2_BLOCK_MASK)
#define GET_VA_OFFSET_L3(va)      ((va) & L3_PAGE_MASK)

// clang-format off
/* table format, which cannot be recognized by clang-format 1.10 */
typedef union {
        struct {
                u64 is_valid        : 1,
                    is_table        : 1,
                    ignored1        : 10,
                    next_table_addr : 36,
                    reserved        : 4,
                    ignored2        : 7,
                    PXNTable        : 1,   // Privileged Execute-never for next level
                    XNTable         : 1,   // Execute-never for next level
                    APTable         : 2,   // Access permissions for next level
                    NSTable         : 1;
        } table;
        struct {
                u64 is_valid        : 1,
                    is_table        : 1,
                    attr_index      : 3,   // Memory attributes index
                    NS              : 1,   // Non-secure
                    AP              : 2,   // Data access permissions
                    SH              : 2,   // Shareability
                    AF              : 1,   // Accesss flag
                    nG              : 1,   // Not global bit
                    reserved1       : 4,
                    nT              : 1,
                    reserved2       : 13,
                    pfn             : 18,
                    reserved3       : 2,
                    GP              : 1,
                    reserved4       : 1,
                    DBM             : 1,   // Dirty bit modifier
                    Contiguous      : 1,
                    PXN             : 1,   // Privileged execute-never
                    UXN             : 1,   // Execute never
                    soft_reserved   : 4,
                    PBHA            : 4;   // Page based hardware attributes
        } l1_block;
        struct {
                u64 is_valid        : 1,
                    is_table        : 1,
                    attr_index      : 3,   // Memory attributes index
                    NS              : 1,   // Non-secure
                    AP              : 2,   // Data access permissions
                    SH              : 2,   // Shareability
                    AF              : 1,   // Accesss flag
                    nG              : 1,   // Not global bit
                    reserved1       : 4,
                    nT              : 1,
                    reserved2       : 4,
                    pfn             : 27,
                    reserved3       : 2,
                    GP              : 1,
                    reserved4       : 1,
                    DBM             : 1,   // Dirty bit modifier
                    Contiguous      : 1,
                    PXN             : 1,   // Privileged execute-never
                    UXN             : 1,   // Execute never
                    soft_reserved   : 4,
                    PBHA            : 4;   // Page based hardware attributes
        } l2_block;
        struct {
                u64 is_valid        : 1,
                    is_page         : 1,
                    attr_index      : 3,   // Memory attributes index
                    NS              : 1,   // Non-secure
                    AP              : 2,   // Data access permissions
                    SH              : 2,   // Shareability
                    AF              : 1,   // Accesss flag
                    nG              : 1,   // Not global bit
                    pfn             : 36,
                    reserved        : 3,
                    DBM             : 1,   // Dirty bit modifier
                    Contiguous      : 1,
                    PXN             : 1,   // Privileged execute-never
                    UXN             : 1,   // Execute never
                    soft_reserved   : 4,
                    PBHA            : 4,   // Page based hardware attributes
                    ignored         : 1;
        } l3_page;
        u64 pte;
} pte_t;
// clang-format on

#define PTE_DESCRIPTOR_INVALID                    (0)

/* page_table_page type */
typedef struct {
	pte_t ent[PTP_ENTRIES];
} ptp_t;

#endif /* ARCH_AARCH64_ARCH_MM_PAGE_TABLE_H */