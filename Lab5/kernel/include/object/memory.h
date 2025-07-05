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

#ifndef OBJECT_MEMORY_H
#define OBJECT_MEMORY_H

#include <common/radix.h>
#include <object/cap_group.h>
#include <uapi/memory.h>
#ifdef CHCORE_OPENTRUSTEE
#include <common/tee_uuid.h>
#endif /* CHCORE_OPENTRUSTEE */

#ifdef CHCORE_OPENTRUSTEE
struct tee_shm_private {
        struct tee_uuid uuid;
        struct cap_group *owner;
};
#endif /* CHCORE_OPENTRUSTEE */

/* This struct represents some physical memory resource */
struct pmobject {
        paddr_t start;
        size_t size;
        pmo_type_t type;
        /* record physical pages for on-demand-paging pmo */
        struct radix *radix;
        /*
         * The field of 'private' depends on 'type'.
         * PMO_FILE: it points to fmap_fault_pool
         * others: NULL
         */
        void *private;
        struct list_head mapping_list;
        /* Lab5, counting page faults */
        long page_faults;
};

/* kernel internal interfaces */
cap_t create_pmo(size_t size, pmo_type_t type, struct cap_group *cap_group,
                 paddr_t paddr, struct pmobject **new_pmo, cap_right_t rights);
void commit_page_to_pmo(struct pmobject *pmo, unsigned long index, paddr_t pa);
paddr_t get_page_from_pmo(struct pmobject *pmo, unsigned long index);
int map_pmo_in_current_cap_group(cap_t pmo_cap, unsigned long addr,
                                 unsigned long perm);
void pmo_deinit(void *pmo_ptr);

/* syscalls */
cap_t sys_create_pmo(unsigned long size, pmo_type_t type, unsigned long val, cap_right_t rights);
int sys_write_pmo(cap_t pmo_cap, unsigned long offset, unsigned long user_ptr, unsigned long len);
int sys_read_pmo(cap_t pmo_cap, unsigned long offset, unsigned long user_ptr, unsigned long len);
int sys_get_phys_addr(vaddr_t va, paddr_t *pa_buf);
int sys_map_pmo(cap_t target_cap_group_cap, cap_t pmo_cap, unsigned long addr, unsigned long perm, unsigned long len);
int sys_unmap_pmo(cap_t target_cap_group_cap, cap_t pmo_cap,
                  unsigned long addr, size_t size);
unsigned long sys_handle_brk(unsigned long addr, unsigned long heap_start);
int sys_handle_mprotect(unsigned long addr, unsigned long length, int prot);
int sys_get_free_mem_size(struct free_mem_info *info);


#endif /* OBJECT_MEMORY_H */
