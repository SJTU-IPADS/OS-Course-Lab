/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <stdio.h>
#include <unistd.h>
#include <chcore/ipc.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <chcore-graphic/gui_defs.h>
#include <chcore-internal/procmgr_defs.h>

#include <g2d/g2d.h>
#include <libpipe.h>
#include "chcore_backend.h"
#include "iomgr.h"
#include "wayland_backend.h"
#include "winmgr.h"
#include "copy_folder.h"

struct gui_server {
        PBITMAP screen_bitmap;
        struct pipe *keyboard_pipe;
        struct pipe *mouse_pipe;
};

static struct gui_server gui_srv = {0};

static void init_gui_srv(void)
{
        init_winmgr();
        init_chcore_backend();
        init_wayland_backend();
        init_iomgr();
}

DEFINE_SERVER_HANDLER(gui_dispatch)
{
        struct gui_request *req;

        /* Get request */
        req = (struct gui_request *)ipc_get_msg_data(ipc_msg);

        switch (req->req) {
        case GUI_CONN:
                if (req->client_type == GUI_CLIENT_CHCORE)
                        chcore_conn(ipc_msg, client_badge);
                else if (req->client_type == GUI_CLIENT_WAYLAND)
                        wayland_conn(ipc_msg, client_badge);
                break;
        default:
                printf("[GUI] Invalid req: %d\n", req->req);
                break;
        }

        /* Shouldn't reach here !*/
        ipc_return(ipc_msg, -1);
}

static void gui_probe_hdmi(void)
{
        if (gui_srv.screen_bitmap == NULL) {
                gui_srv.screen_bitmap = iomgr_get_screen_bitmap();
                if (gui_srv.screen_bitmap != NULL) {
                        wm_set_screen_bitmap(gui_srv.screen_bitmap);
                        wm_set_screen_size(
                                get_bitmap_width(gui_srv.screen_bitmap),
                                get_bitmap_height(gui_srv.screen_bitmap));
                }
        }
}

static void gui_probe_keyboard(void)
{
        if (gui_srv.keyboard_pipe == NULL) {
                gui_srv.keyboard_pipe = iomgr_get_keyboard_pipe();
                if (gui_srv.keyboard_pipe != NULL) {
                        wayland_add_keyboard_pipe(gui_srv.keyboard_pipe);
                }
        }
}

static void gui_probe_mouse(void)
{
        if (gui_srv.mouse_pipe == NULL) {
                gui_srv.mouse_pipe = iomgr_get_mouse_pipe();
                if (gui_srv.mouse_pipe != NULL) {
                        wayland_add_mouse_pipe(gui_srv.mouse_pipe);
                }
        }
}

int main(int argc, char *argv[], char *envp[])
{
        int ret;

        copy_directory("/sys/gui/cursors", "/cursors");

        /* Init GUI server */
        init_gui_srv();

        /*
         * Register client handler:
         * GUI server is a single-handler-thread server.
         */
        ret = ipc_register_server(gui_dispatch, register_cb_single);
        printf("[GUI] register server value = %d\n", ret);

        printf("[GUI] main loop...\n"); // debug
        while (true) {
                gui_probe_hdmi();
                gui_probe_keyboard();
                gui_probe_mouse();
                chcore_handle_backend();
                wayland_handle_backend();
                wm_comp(); // do window composition
                chcore_awake_clients();
                usleep(16000);
        }
        return 0;
}
