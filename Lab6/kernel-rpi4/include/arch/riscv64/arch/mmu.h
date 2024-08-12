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

#pragma once

#include <common/types.h>
#include <common/macro.h>
#include <common/vars.h>
#include <arch/mm/page_table.h>
#include <uapi/memory.h>

/* functions */
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa,
		       size_t len, vmr_prop_t flags, long *rss);
int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, long *rss);
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry);
int mprotect_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, vmr_prop_t prop);

#ifdef CHCORE

#define phys_to_virt(x) ((vaddr_t)((paddr_t)(x) + KBASE))
#define virt_to_phys(x) ((paddr_t)((vaddr_t)(x) - KBASE))

#ifndef KCODE

#if defined(SV48)
#define KCODE 0xFFFFFF0180000000 // ChCore now only map 0 ~ 0x180000000(6G) of physical address space.
#elif defined(SV39)
#ifdef CHCORE_PLAT_VISIONFIVE2
#define KCODE 0xFFFFFFF180000000 // VisionFive2 board map 0 ~ 0x180000000 of physical address space.
#else
#define KCODE 0xFFFFFFF180000000 // Qemu virt map 0 ~ 0x180000000 of physical address space.
#endif
#else
#error "SV32 not supported."
#endif

#endif

#endif
