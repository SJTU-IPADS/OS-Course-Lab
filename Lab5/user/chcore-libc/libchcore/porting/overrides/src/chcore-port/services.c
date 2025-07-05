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

#include <chcore/services.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

ipc_struct_t *chcore_conn_srv(const char *service_name)
{
        extern ipc_struct_t *procmgr_ipc_struct;
        ipc_msg_t *ipc_msg;
        struct proc_request *pr;
        cap_t srv_cap;
        s64 ret;

        /* Get server cap */
        ipc_msg = ipc_create_msg(
                procmgr_ipc_struct, sizeof(struct proc_request));
        pr = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        pr->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(pr->get_service_cap.service_name, service_name, SERVICE_NAME_LEN);
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        if (ret < 0) {
                ipc_destroy_msg(ipc_msg);
                return NULL;
        }
        srv_cap = ipc_get_msg_cap(ipc_msg, 0);
        ipc_destroy_msg(ipc_msg);

        /* Register client */
        return ipc_register_client(srv_cap);
}

#ifdef __cplusplus
}
#endif