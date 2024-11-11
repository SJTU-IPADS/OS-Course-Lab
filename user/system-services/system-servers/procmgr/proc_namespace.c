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


#include <chcore-internal/fs_defs.h>
#include <chcore/ipc.h>
#include <stdio.h>
#include <string.h>
#include <chcore/file_namespace.h>

#include "proc_namespace.h"

int new_file_namespace(badge_t cur_badge, badge_t parent_badge,
                       struct user_file_namespace *fs_ns);
int del_file_namespace(badge_t cur_badge);

int new_namespace(badge_t cur_badge, badge_t parent_badge, int ns_flags,
                  struct proc_ns *ns)
{
        int ret;

        if (ns_flags & FILE_NS) {
                ret = new_file_namespace(cur_badge, parent_badge, proc_ns_fs(ns));
        } else {
                ret = new_file_namespace(cur_badge, parent_badge, NULL);
        }

        return ret;
}


int del_namespace(badge_t cur_badge)
{
        del_file_namespace(cur_badge);
        return 0;
}


int new_file_namespace(badge_t cur_badge, badge_t parent_badge,
                       struct user_file_namespace *fs_ns)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        long ret;

        /* Create IPC message */
        ipc_msg = ipc_create_msg(fsm_ipc_struct, sizeof(struct fsm_request));
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        fsm_req->req = FSM_REQ_NEW_NAMESPACE;
        fsm_req->new_ns.parent_badge = parent_badge;
        fsm_req->new_ns.cur_badge = cur_badge;

        if (fs_ns == NULL) { // No specific file namespace
                fsm_req->new_ns.with_config = 0;
        } else {
                fsm_req->new_ns.with_config = 1;
                memcpy(&fsm_req->new_ns.fs_ns, fs_ns, sizeof(*fs_ns));
        }

        /* Send IPC to FSM */
        ret = ipc_call(fsm_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int del_file_namespace(badge_t cur_badge)
{
        ipc_msg_t *ipc_msg;
        struct fsm_request *fsm_req;
        int ret;

        /* Create IPC message */
        ipc_msg = ipc_create_msg_with_cap(fsm_ipc_struct, sizeof(struct fsm_request), 1);
        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        fsm_req->req = FSM_REQ_DELETE_NAMESPACE;
        fsm_req->del_ns.badge = cur_badge;
        ret = ipc_call(fsm_ipc_struct, ipc_msg);

        ipc_destroy_msg(ipc_msg);

        return ret;
}
