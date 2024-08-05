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

#ifndef SHELL_MSG_HANDLER_H
#define SHELL_MSG_HANDLER_H

#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>

extern int shell_is_waiting;
extern cap_t shell_cap;
extern int shell_pid;

void handle_recv_sig(ipc_msg_t *ipc_msg, struct proc_request *pr);
void handle_check_state(ipc_msg_t *ipc_msg, badge_t client_badge,
                        struct proc_request *pr);
void handle_get_shell_cap(ipc_msg_t *ipc_msg);
void handle_set_shell_cap(ipc_msg_t *ipc_msg, badge_t client_badge);
void handle_get_terminal_cap(ipc_msg_t *ipc_msg);
void handle_set_terminal_cap(ipc_msg_t *ipc_msg);

#endif /* SHELL_MSG_HANDLER_H */