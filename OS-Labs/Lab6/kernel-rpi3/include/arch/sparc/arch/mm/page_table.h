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

#define PAGE_SHIFT              (12)
#define PAGE_TABLE_SHIFT        (6)
#define PAGE_MASK		((1 << PAGE_SHIFT) - 1)

#define ENTRY_TYPE_MASK         (3)
#define ENTRY_TYPE_INVALID      (0)
#define ENTRY_TYPE_PTD          (1)
#define ENTRY_TYPE_PTE          (2)

#define ACC_WRITE_BIT           (1)
#define ACC_EXEC_BIT            (2)

#define ACC_SHIFT		(2)
#define ACC_READ_ONLY           (0)
#define ACC_READ_WRITE          (ACC_WRITE_BIT)
#define ACC_READ_EXEC           (ACC_EXEC_BIT)
#define ACC_READ_WRITE_EXEC     (ACC_WRITE_BIT | ACC_EXEC_BIT)
#define ACC_EXEC_ONLY           (4)
/* Supervisor access */
#define ACC_READ_WRITE_S	(5)
#define ACC_READ_EXEC_S		(6)
#define ACC_READ_WRITE_EXEC_S	(7)

#define CACHEABLE               (1 << 7)

#define L1_PAGE_ORDER   (8)
#define L2_PAGE_ORDER   (6)
#define L3_PAGE_ORDER   (6)

#define L1_PTP_ENTRIES  (1UL << L1_PAGE_ORDER)
#define L2_PTP_ENTRIES  (1UL << L2_PAGE_ORDER)
#define L3_PTP_ENTRIES  (1UL << L3_PAGE_ORDER)

#define L1_PTP_INDEX_MASK               ((1 << (L1_PAGE_ORDER)) - 1)
#define L2_PTP_INDEX_MASK               ((1 << (L2_PAGE_ORDER)) - 1)
#define L3_PTP_INDEX_MASK               ((1 << (L3_PAGE_ORDER)) - 1)

#define L1_PT_SIZE      (L1_PTP_ENTRIES * sizeof(long))
#define L2_3_PT_SIZE    (L2_PTP_ENTRIES * sizeof(long))

#define L1_INDEX_SHIFT                  (L2_PAGE_ORDER + L3_PAGE_ORDER + PAGE_SHIFT)
#define L2_INDEX_SHIFT                  (L3_PAGE_ORDER + PAGE_SHIFT)
#define L3_INDEX_SHIFT                  PAGE_SHIFT

#define GET_L1_INDEX(addr) (((addr) >> L1_INDEX_SHIFT) & L1_PTP_INDEX_MASK)
#define GET_L2_INDEX(addr) (((addr) >> L2_INDEX_SHIFT) & L2_PTP_INDEX_MASK)
#define GET_L3_INDEX(addr) (((addr) >> L3_INDEX_SHIFT) & L3_PTP_INDEX_MASK)

#define L1_PER_ENTRY_PAGES	  ((L2_PTP_ENTRIES) * (L2_PER_ENTRY_PAGES))
#define L2_PER_ENTRY_PAGES	  ((L3_PTP_ENTRIES) * (L3_PER_ENTRY_PAGES))
#define L3_PER_ENTRY_PAGES	  (1)

#define L1_BLOCK_MASK   ((L1_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)
#define L2_BLOCK_MASK   ((L2_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)
#define L3_PAGE_MASK    ((L3_PER_ENTRY_PAGES << PAGE_SHIFT) - 1)

#define GET_VA_OFFSET_L1(va)      ((va) & L1_BLOCK_MASK)
#define GET_VA_OFFSET_L2(va)      ((va) & L2_BLOCK_MASK)
#define GET_VA_OFFSET_L3(va)      ((va) & L3_PAGE_MASK)

// clang-format off
typedef union {
        struct {
                unsigned int
                        page_table_ptr      : 30,
                        entry_type          : 2;
        } ptd;
        struct {
                unsigned int
                        ppn                 : 24,
                        cacheable           : 1,
                        modified            : 1,
                        referenced          : 1,
                        acc                 : 3,
                        entry_type          : 2;
        } pte;
        unsigned int val;
} pte_t;
// clang-format on

#define PTE_DESCRIPTOR_INVALID                    (0)

typedef struct {
	pte_t ent[L1_PTP_ENTRIES];
} ptp_t;
