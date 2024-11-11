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

#ifndef MEM_OPS_H
#define MEM_OPS_H

#include <stdint.h>
#include "tee_uuid.h"

#define STATPROC_NAME_MAX 64
#define PATHNAME_MAX      256
#define STATPROC_MAX      100

struct stat_proc_mem {
        char name[STATPROC_NAME_MAX];
        uint32_t mem;
        uint32_t mem_max;
        uint32_t mem_limit;
};

struct stat_mem_info {
        uint32_t total_mem;
        uint32_t mem;
        uint32_t free_mem;
        uint32_t free_mem_min;
        uint32_t proc_num;
        struct stat_proc_mem proc_mem[STATPROC_MAX];
};

int mem_ops_init(void);

int32_t task_map_ns_phy_mem(uint32_t task_id, uint64_t phy_addr, uint32_t size,
                            uint64_t *info);

int32_t self_map_ns_phy_mem(uint64_t info, uint32_t size, uint64_t *virt_addr);

int32_t task_unmap(uint32_t task_id, uint64_t info, uint32_t size);

int32_t self_unmap(uint64_t info);

void *alloc_sharemem_aux(const struct tee_uuid *uuid, uint32_t size);

uint32_t free_sharemem(void *addr, uint32_t size);

int32_t map_sharemem(uint32_t src_task, uint64_t vaddr, uint64_t size,
                     uint64_t *vaddr_out);

uint64_t virt_to_phys(uintptr_t vaddr);

int32_t dump_mem_info(struct stat_mem_info *info, int print_history);

#endif /* MEM_OPS_H */
