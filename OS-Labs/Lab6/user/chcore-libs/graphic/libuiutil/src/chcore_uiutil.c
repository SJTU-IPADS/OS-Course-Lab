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

#include <chcore_uiutil.h>

#include <chcore/ipc.h>
#include <chcore/syscall.h>
#include <chcore/services.h>
#include <chcore-graphic/gui_defs.h>
#include <chcore-internal/procmgr_defs.h>

#include <libpipe.h>

struct win_conn {
        ipc_struct_t *gui_ipc_struct;
        cap_t vsync_notific_cap;
        struct pipe *c2s_pipe;
        struct pipe *s2c_pipe;
};

static struct win_conn wconn = {0};

/* Return 0 if succ */
int connect_to_gui_server(void)
{
        ipc_msg_t *gui_msg;
        struct gui_request *req;
        cap_t c2s_pipe_pmo, s2c_pipe_pmo;
        int ret;

        if (wconn.gui_ipc_struct != NULL)
                return -1;

        /* Connect to GUI server */
        wconn.gui_ipc_struct = chcore_conn_srv(SERVER_GUI);
        if (wconn.gui_ipc_struct == NULL)
                return -1;

        wconn.vsync_notific_cap = usys_create_notifc();
        c2s_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);
        s2c_pipe_pmo = usys_create_pmo(PAGE_SIZE, PMO_DATA);

        gui_msg = ipc_create_msg_with_cap(wconn.gui_ipc_struct, sizeof(struct gui_request), 4);
        req = (struct gui_request *)ipc_get_msg_data(gui_msg);
        req->req = GUI_CONN;
        req->client_type = GUI_CLIENT_CHCORE;
        ipc_set_msg_cap(gui_msg, 0, SELF_CAP);
        ipc_set_msg_cap(gui_msg, 1, c2s_pipe_pmo);
        ipc_set_msg_cap(gui_msg, 2, s2c_pipe_pmo);
        ipc_set_msg_cap(gui_msg, 3, wconn.vsync_notific_cap);

        ret = ipc_call(wconn.gui_ipc_struct, gui_msg);
        if (ret != 0) {
                ipc_destroy_msg(gui_msg);
                return -1;
        }

        wconn.c2s_pipe = create_pipe_from_pmo(PAGE_SIZE, c2s_pipe_pmo);
        wconn.s2c_pipe = create_pipe_from_pmo(PAGE_SIZE, s2c_pipe_pmo);

        ipc_destroy_msg(gui_msg);
        return 0;
}
