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

#ifndef MM_MM_H
#define MM_MM_H

#include <common/util.h>
#include <mm/vmspace.h>
#include <mm/buddy.h>

#ifndef PAGE_SIZE
#define PAGE_SIZE BUDDY_PAGE_SIZE
#endif

extern void parse_mem_map(void *); 

/* Execute once during kernel init. */
void mm_init(void* physmem_info);

/* Return the size of free memory in the buddy and slab allocator. */
unsigned long get_free_mem_size(void);

/* Implementations differ on different architectures. */
void set_page_table(paddr_t pgtbl);
void flush_tlb_by_range(struct vmspace*, vaddr_t start_va, size_t size);
void flush_tlb_by_vmspace(struct vmspace *);
void flush_idcache(void);

/* Only needed on SPARC */
void sys_cache_config(unsigned option);
void plat_cache_config(unsigned option);

#endif /* MM_MM_H */