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

/*
 *	aarch64 kernel virtual memory mapping layout (use raspi3 as example)
 *    +---DIRECT MAPPING_start---+  0xFFFFFF0000000000 (KBASE)
 *    |       kernel code        |
 *    |                          |
 *    +--------------------------+  img_end
 *    |         kmalloc          |
 *    |                          |
 *    +--------------------------+  0xFFFFFF003F000000 (KBASE + PERIPHERAL_BASE)
 *    |  peripheral mapping      |
 *    |                          |
 *    +----DIRECT MAPPING_end----+  0xFFFFFF0040000000 (KBASE + PERIPHERAL_END)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00000000 (KSTACK_BASE)
 *    |       kernel stack       |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFF00008000 (KSTACK_END)
 *    |        ......            |
 *    |                          |
 *    +--------------------------+  0xFFFFFFFFFFFFFFFF
*/

#ifndef ARCH_AARCH64_ARCH_MMU_H
#define ARCH_AARCH64_ARCH_MMU_H

#include <common/vars.h>

#ifndef KBASE
#define KBASE 0xFFFFFF0000000000
#define PHYSICAL_ADDR_MASK (40)
#endif // KBASE

#ifndef KSTACK_BASE
#define KSTACK_BASE 0xFFFFFFFF00000000
#define KSTACKx_ADDR(cpuid) ((cpuid) * 2 * CPU_STACK_SIZE + KSTACK_BASE)
#endif // KSTACK

#ifndef __ASM__

#include <arch/mm/page_table.h>
#include <common/types.h>
#include <uapi/memory.h>

/* functions */
int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags);
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags);
int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len);
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry);
int mprotect_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, vmr_prop_t prop);
void set_ttbr0_el1(paddr_t ttbr0);

struct vmspace;
void flush_tlb_opt(struct vmspace *vmspace, vaddr_t addr, size_t size);
void flush_tlb_all(void);

#ifdef CHCORE

#define phys_to_virt(x) ((vaddr_t)((paddr_t)(x) + KBASE))
#define virt_to_phys(x) ((paddr_t)((vaddr_t)(x) - KBASE))

#endif // CHCORE

#endif // __ASM__

#endif /* ARCH_AARCH64_ARCH_MMU_H */