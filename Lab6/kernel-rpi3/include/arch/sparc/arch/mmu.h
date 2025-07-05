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
#include <arch/mm/page_table.h>
#ifdef CHCORE_PLAT_LEON3
#include <plat/leon3/mm.h>
#elif defined CHCORE_PLAT_BM3823
#include <plat/bm3823/mm.h>
#else
// #define KERNEL_PHY_START 0x0
#endif
#include <uapi/memory.h>

#ifndef KBASE
#define KBASE 0xC0000000
#endif

#ifndef KSTACK_BASE
#define KSTACK_BASE (0xE0000000UL)
#define KSTACKx_ADDR(cpuid) ((cpuid) * 2 * CPU_STACK_SIZE + KSTACK_BASE)
#define KSTACK_END  (0xE1000000UL)
#endif // KSTACK

struct vmspace;

#define MMU_BOOT_CTX	0
#define MMU_CTRL_REG		(0x00000000UL)
#define MMU_CTX_PTR_REG		(0x00000100UL)
#define MMU_CTX_REG		(0x00000200UL)
#define MMU_FAULT_STAT_REG	(0x00000300UL)
#define MMU_FAULT_ADDR_REG	(0x00000400UL)

/* MMU Fault Status Register bits */
#define MFSR_FT_SHIFT		(2)
#define MFSR_FT_MASK		(7)
#define MFSR_FAV_SHIFT		(1)
#define MFSR_FAV_MASK		(1)
#define MFSR_AT_SHIFT       (5)
#define MFSR_AT_MASK        (7)
/* Fault type */
#define MFSR_FT_INVA_ADDR_ERR	(1)
#define MFSR_FT_PROTECTION_ERR	(2)
#define MFSR_FT_PRIVILEGE_ERR	(3)
/* Access type */
#define MFSR_AT_LOAD_USER_DATA              (0)
#define MFSR_AT_LOAD_SUPERVISOR_DATA        (1)
#define MFSR_AT_LOAD_EXEC_USER_INSTR        (2)
#define MFSR_AT_LOAD_EXEC_SUPERVISOR_INSTR  (3)
#define MFSR_AT_STORE_USER_DATA             (4)
#define MFSR_AT_STORE_SUPERVISOR_DATA       (5)
#define MFSR_AT_STORE_USER_INSTR            (6)
#define MFSR_AT_STORE_SUPERVISOR_INSTR      (7)

int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags);
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags, long *rss);
int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, long *rss);
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry);
int mprotect_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, vmr_prop_t prop);

#define KERNEL_START	(0xC0000000UL)
#define KERNEL_END	(0xE0000000UL)
#define KOFFSET		(KERNEL_START - KERNEL_PHY_START)

#define phys_to_virt(x) ((vaddr_t)((paddr_t)(x) + KOFFSET))
#define virt_to_phys(x) ((paddr_t)((vaddr_t)(x) - KOFFSET))

/* MMU flush */
#define FLUSH_ADDR_ORDER	(12)
#define FLUSH_TYPE_PAGE		(0UL << 8)
#define FLUSH_TYPE_SEGMENT	(1UL << 8)
#define FLUSH_TYPE_REGION	(2UL << 8)
#define FLUSH_TYPE_CONTEXT	(3UL << 8)
#define FLUSH_TYPE_ENTIRE	(4UL << 8)

/*
 * Size of the context table
 * TODO: Check if the size is enough for use
 */
#define CTX_NUMBER 256
