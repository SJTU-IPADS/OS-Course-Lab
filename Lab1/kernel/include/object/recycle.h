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

#ifndef OBJECT_RECYCLE_H
#define OBJECT_RECYCLE_H

#include <common/types.h>

/* Syscalls */
int sys_register_recycle(cap_t notifc_cap, vaddr_t msg_buffer);
void sys_exit_group(int exitcode);
int sys_kill_group(int proc_cap);
int sys_cap_group_recycle(cap_t cap_group_cap);
int sys_ipc_close_connection(cap_t connection_cap);

#endif /* OBJECT_RECYCLE_H */