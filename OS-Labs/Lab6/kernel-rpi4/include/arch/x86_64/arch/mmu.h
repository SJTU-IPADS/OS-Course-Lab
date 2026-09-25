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

/* TODO: This file seems to be general */

/*
 *	x86_64 kernel virtual memory mapping layout
 *    +---DIRECT MAPPING_start---+  0xFFFFFF0000000000 (KBASE)
 *    |       kernel img         |
 *    |                          |
 *    +--------------------------+  img_end
 *    |         kmalloc          |
 *    |                          |
 *    +----DIRECT MAPPING_end----+  (KBASE + available memory size)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00000000 (KSTACK_BASE)
 *    |     kernel stack         |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00008000 (KSTACK_END)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFFC0000000 (KCODE)
 *    |     kernel code          |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFFFFFFFFFF
*/

#pragma once

#include <common/types.h>
#include <common/macro.h>
#include <common/vars.h>
#include <arch/mm/page_table.h>
#include <uapi/memory.h>

struct vmspace;

/* functions */
int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags);
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags, long *rss);
int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, long *rss);
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry);
int mprotect_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, vmr_prop_t prop);
void flush_tlb_local_and_remote(struct vmspace* vmspace,
				vaddr_t start_va, size_t len);
void invpcid_flush_single_context(u64 pcid);
void flush_local_tlb_opt(vaddr_t start_va, u64 page_cnt, u64 pcid);
void flush_tlb_all(void);

#ifndef KBASE
#define KBASE 0xFFFFFF0000000000
#endif

#ifndef KSTACK_BASE
#define KSTACK_BASE 0xFFFFFFFF00000000
#define KSTACKx_ADDR(cpuid) ((cpuid) * 2 * CPU_STACK_SIZE + KSTACK_BASE)
#endif // KSTACK

#ifdef CHCORE

#define phys_to_virt(x) ((vaddr_t)((paddr_t)(x) + KBASE))
#define virt_to_phys(x) ((paddr_t)((vaddr_t)(x) - KBASE))

#ifndef KCODE
#define KCODE 0xFFFFFFFFC0000000
#endif

#endif
