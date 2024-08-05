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

#ifndef SYSCALL_SYSCALL_HOOKS_H
#define SYSCALL_SYSCALL_HOOKS_H

#include <common/types.h>

int hook_sys_create_pmo(unsigned long size, pmo_type_t type, unsigned long val);
int hook_sys_get_phys_addr(vaddr_t va, paddr_t *pa_buf);
int hook_sys_create_cap_group(unsigned long cap_group_args_p);
int hook_sys_register_recycle(cap_t notifc_cap, vaddr_t msg_buffer);
int hook_sys_cap_group_recycle(cap_t cap_group_cap);
int hook_sys_kill_group(cap_t proc_cap);

#endif /* SYSCALL_SYSCALL_HOOKS_H */