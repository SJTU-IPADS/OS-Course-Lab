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

#include <chcore-internal/procmgr_defs.h>
#include <uapi/get_system_info.h>
#include <malloc.h>
#include <stdio.h>

int main(void)
{
        ipc_msg_t *ipc_msg;
	struct proc_request *proc_req;

        /* use ipc to procmgr to get process id */
        ipc_msg = ipc_create_msg(procmgr_ipc_struct, sizeof *proc_req);
        proc_req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        proc_req->req = PROC_REQ_GET_SYSTEM_INFO;
        ipc_call(procmgr_ipc_struct, ipc_msg);

        ipc_destroy_msg(ipc_msg);
        return 0;
}