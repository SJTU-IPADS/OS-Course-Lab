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

#include <errno.h>
#include <signal.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/ipc.h>

int chcore_kill(pid_t pid, int sig)
{
        struct proc_request *proc_req;
        ipc_msg_t *proc_ipc_msg;
        int ret;

        proc_ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        proc_req = (struct proc_request *)ipc_get_msg_data(proc_ipc_msg);
        proc_req->req = PROC_REQ_KILL;
        proc_req->kill.pid = pid;

        ret = ipc_call(procmgr_ipc_struct, proc_ipc_msg);
        ipc_destroy_msg(proc_ipc_msg);
        if (ret < 0) {
                errno = -ret;
                return -1;
        }
        return 0;
}
