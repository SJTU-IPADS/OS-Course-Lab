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

#include <chcore/type.h>
#include <uapi/memory.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Allocate a continous virtual address region whose length
 * is at least @size. And if @size is not aligned to 4K, the actual
 * allocated size is rounded up to 4K-aligned.
 *
 * @param size
 * @return start address of allocated virtual address region if success,
 * 	   0 if failed.
 */
unsigned long chcore_alloc_vaddr(unsigned long size);

/**
 * @brief Free all virtual address regions **completely** enclosed by
 * [vaddr, vaddr + size).
 *
 * !! [WARNING]: we don't support split virtual address regions currently,
 * so if there is a region just overlapped with [vaddr, vaddr + size), it
 * would not be freed.
 *
 * @param vaddr
 * @param size
 */
void chcore_free_vaddr(unsigned long vaddr, unsigned long size);

/*
 * Automatically map and unmap PMO to available virtual address.
 */
void *chcore_auto_map_pmo(cap_t pmo, unsigned long size, unsigned long perm);
void chcore_auto_unmap_pmo(cap_t pmo, unsigned long vaddr, unsigned long size);

struct chcore_dma_handle {
        cap_t pmo;
        unsigned long size;
        unsigned long vaddr;
        unsigned long paddr;
};
/*
 * Allocate and free coherent memory (consecutive & non-cacheable) for DMA.
 */
void *chcore_alloc_dma_mem(unsigned long size,
                           struct chcore_dma_handle *dma_handle);
void chcore_free_dma_mem(struct chcore_dma_handle *dma_handle);

#ifdef __cplusplus
}
#endif
