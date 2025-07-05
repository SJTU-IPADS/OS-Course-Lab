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

#include <fcntl.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/shell_defs.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>

static ipc_struct_t *shell_msg_struct = NULL;
static cap_t shell_server_cap = -1;

void shell_new_fg_proc(pid_t pid) {
	ipc_msg_t *shell_msg;
	ipc_msg_t *procmgr_msg;
	struct proc_request *proc_req;
	struct shell_req *req;

	if (shell_msg_struct == NULL) {
		procmgr_msg = ipc_create_msg(
				procmgr_ipc_struct, sizeof(struct proc_request));
		proc_req = (struct proc_request *)ipc_get_msg_data(procmgr_msg);

		proc_req->req = PROC_REQ_GET_SHELL_CAP;
		ipc_call(procmgr_ipc_struct, procmgr_msg);
		BUG_ON(ipc_get_msg_return_cap_num(procmgr_msg) == 0);

		shell_server_cap = ipc_get_msg_cap(procmgr_msg, 0);
		BUG_ON(shell_server_cap < 0);
		ipc_destroy_msg(procmgr_msg);

		shell_msg_struct = ipc_register_client(shell_server_cap);
		BUG_ON(shell_msg_struct == NULL);
	}
	shell_msg = ipc_create_msg_with_cap(
			shell_msg_struct, sizeof(struct shell_req), 2);
	req = (struct shell_req *)ipc_get_msg_data(shell_msg);
	req->pid = pid;
	req->req = SHELL_SET_FG_PROCESS;
	ipc_call(shell_msg_struct, shell_msg);
	ipc_destroy_msg(shell_msg);
}

void shell_fg_proc_return(pid_t pid) {
	ipc_msg_t *shell_msg;
	struct shell_req *req;

	BUG_ON(shell_msg_struct == NULL);
	shell_msg = ipc_create_msg_with_cap(
			shell_msg_struct, sizeof(struct shell_req), 2);
	req = (struct shell_req *)ipc_get_msg_data(shell_msg);
	req->pid = pid;
	req->req = SHELL_FG_PROCESS_RETURN;
	ipc_call(shell_msg_struct, shell_msg);
	ipc_destroy_msg(shell_msg);
}