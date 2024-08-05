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

#ifndef MM_PAGE_FAULT_H
#define MM_PAGE_FAULT_H

#include <mm/vmspace.h>
#include <common/types.h>
#include <arch/mmu.h>

int handle_trans_fault(struct vmspace *vmspace, vaddr_t fault_addr);

int handle_perm_fault(struct vmspace *vmspace, vaddr_t fault_addr,
                      vmr_prop_t desired_perm);

#endif /* MM_PAGE_FAULT_H */