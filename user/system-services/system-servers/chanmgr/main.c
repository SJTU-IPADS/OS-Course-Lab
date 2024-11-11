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

#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore-internal/chanmgr_defs.h>
#include <errno.h>
#include <chanmgr.h>

DEFINE_SERVER_HANDLER_TASKID(chanmgr_dispatch)
{
        struct chan_request *req;

        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);

        switch (req->req) {
        case CHAN_REQ_CREATE_CHANNEL:
                chanmgr_handle_create_channel(ipc_msg, pid, pid, tid);
                break;
        case CHAN_REQ_REMOVE_CHANNEL:
                chanmgr_handle_remove_channel(ipc_msg, pid, pid, tid);
                break;
        case CHAN_REQ_HUNT_BY_NAME:
                chanmgr_handle_hunt_by_name(ipc_msg, pid, tid);
                break;
        case CHAN_REQ_GET_CH_FROM_PATH:
                chanmgr_handle_get_ch_from_path(ipc_msg, pid, tid);
                break;
        case CHAN_REQ_GET_CH_FROM_TASKID:
                chanmgr_handle_get_ch_from_taskid(ipc_msg, pid, tid);
                break;
        default:
                ipc_return(ipc_msg, -EBADRQC);
                break;
        }
}

int main(void)
{
        int ret;

        chanmgr_init();

        ret = ipc_register_server_with_destructor(
                chanmgr_dispatch,
                DEFAULT_CLIENT_REGISTER_HANDLER,
                chanmgr_destructor);
        printf("[chanmgr] register server value = %d\n", ret);

        usys_exit(0);
}
