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

#ifndef OH_MEM_OPS_H
#define OH_MEM_OPS_H

#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>

void oh_mem_ops_init(void);

void handle_transfer_pmo_cap(ipc_msg_t *ipc_msg, badge_t badge);

void handle_tee_alloc_sharemem(ipc_msg_t *ipc_msg, badge_t badge);
void handle_tee_get_sharemem(ipc_msg_t *ipc_msg, badge_t badge);
void handle_tee_free_sharemem(ipc_msg_t *ipc_msg, badge_t badge);

void clean_sharemem(badge_t badge);

#endif /* OH_MEM_OPS_H */
