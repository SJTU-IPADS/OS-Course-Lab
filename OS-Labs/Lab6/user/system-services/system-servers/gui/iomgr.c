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

#include "iomgr.h"
#include "guilog/guilog.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <chcore/ipc.h>
#include <chcore/services.h>
#include <chcore-internal/hdmi_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore-internal/usb_devmgr_defs.h>

#include <g2d/g2d.h>
#include <libpipe.h>

struct iomgr {
        pthread_t thread_main_loop;
        ipc_struct_t *hdmi_ipc_struct;
        ipc_struct_t *keyboard_ipc_struct;
        ipc_struct_t *mouse_ipc_struct;
        ipc_struct_t *uspi_ipc_struct;
        ipc_struct_t *circle_ipc_struct;
        PBITMAP sc_bitmap;
        struct pipe *keyboard_pipe;
        struct pipe *mouse_pipe;
};

static struct iomgr iom = {0};

static void iom_get_hdmi(void)
{
        ipc_msg_t *ipc_msg;
        struct hdmi_request *req;
        int ret;
        cap_t pmo_cap;

        GUI_DEBUG("iom_get_hdmi\n"); // debug

        ipc_msg = ipc_create_msg(iom.hdmi_ipc_struct,
                                 sizeof(struct hdmi_request));
        req = (struct hdmi_request *)ipc_get_msg_data(ipc_msg);
        req->req = HDMI_GET_FB;
        req->width = 0;
        req->height = 0;
        req->depth = 0;

        ret = ipc_call(iom.hdmi_ipc_struct, ipc_msg);
        if (ret != 0) {
                ipc_destroy_msg(ipc_msg);
                return;
        }

        pmo_cap = ipc_get_msg_cap(ipc_msg, 0);
        if (pmo_cap < 0) {
                ipc_destroy_msg(ipc_msg);
                return;
        }

        iom.sc_bitmap = create_bitmap_from_pmo(req->ret_width,
                                               req->ret_height,
                                               req->ret_depth,
                                               req->ret_pitch,
                                               pmo_cap);
        ipc_destroy_msg(ipc_msg);

        GUI_LOG("iom_get_hdmi succ\n"); // debug
}

static inline void iom_probe_hdmi(void)
{
        if (iom.hdmi_ipc_struct == NULL) {
                iom.hdmi_ipc_struct = chcore_conn_srv(SERVER_HDMI_DRIVER);
        } else if (iom.sc_bitmap == NULL) {
                iom_get_hdmi();
        }
}

static void iom_get_usb_mouse(void)
{
        ipc_msg_t *ipc_msg;
        struct usb_devmgr_request *uspi_req;
        int ret;
        cap_t pipe_pmo;
        GUI_DEBUG("iom_get_usb_mouse\n");

        ipc_msg = ipc_create_msg(iom.uspi_ipc_struct,
                                 sizeof(struct usb_devmgr_request));
        uspi_req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);
        uspi_req->req = USB_DEVMGR_GET_MOUSE_PIPE;

        ret = ipc_call(iom.uspi_ipc_struct, ipc_msg);
        if (ret != 0) {
                ipc_destroy_msg(ipc_msg);
                return;
        }

        pipe_pmo = ipc_get_msg_cap(ipc_msg, 0);
        if (pipe_pmo < 0) {
                ipc_destroy_msg(ipc_msg);
                GUI_ERROR("iom_get_usb_mouse fail\n");
                return;
        }
        ipc_destroy_msg(ipc_msg);

        iom.mouse_pipe = create_pipe_from_pmo(PAGE_SIZE, pipe_pmo);
        GUI_LOG("iom_get_usb_mouse succ 0x%p\n", iom.mouse_pipe);
}

static void iom_get_usb_keyboard(void)
{
        ipc_msg_t *ipc_msg;
        struct usb_devmgr_request *uspi_req;
        int ret;
        cap_t pipe_pmo;

        GUI_DEBUG("iom_get_usb_keyboard\n"); // debug

        ipc_msg = ipc_create_msg(iom.uspi_ipc_struct,
                                 sizeof(struct usb_devmgr_request));
        uspi_req = (struct usb_devmgr_request *)ipc_get_msg_data(ipc_msg);
        uspi_req->req = USB_DEVMGR_GET_KEYBOARD_PIPE;

        ret = ipc_call(iom.uspi_ipc_struct, ipc_msg);
        if (ret != 0) {
                ipc_destroy_msg(ipc_msg);
                return;
        }

        pipe_pmo = ipc_get_msg_cap(ipc_msg, 0);
        if (pipe_pmo < 0) {
                ipc_destroy_msg(ipc_msg);
                GUI_ERROR("iom_get_usb_keyboard fail\n"); // debug
                return;
        }
        ipc_destroy_msg(ipc_msg);

        iom.keyboard_pipe = create_pipe_from_pmo(PAGE_SIZE, pipe_pmo);
        GUI_LOG("iom_get_usb_keyboard succ 0x%p\n",
               iom.keyboard_pipe); // debug
}

static inline void iom_probe_usb(void)
{
        if (iom.uspi_ipc_struct == NULL) {
                iom.uspi_ipc_struct = chcore_conn_srv(SERVER_USB_DEVMGR);
        } else {
                if (iom.keyboard_pipe == NULL)
                        iom_get_usb_keyboard();
                if (iom.mouse_pipe == NULL)
                        iom_get_usb_mouse();
        }
}

static void *iom_main_loop(void *arg)
{
        GUI_LOG("iom_main_loop...\n"); // debug
        while (true) {
                iom_probe_hdmi();
                iom_probe_usb();
                usleep(200000);
        }
        return NULL;
}

inline void init_iomgr(void)
{
        pthread_create(&iom.thread_main_loop, NULL, iom_main_loop, NULL);
}

inline PBITMAP iomgr_get_screen_bitmap(void)
{
        return iom.sc_bitmap;
}
inline struct pipe *iomgr_get_keyboard_pipe(void)
{
        return iom.keyboard_pipe;
}

inline struct pipe *iomgr_get_mouse_pipe(void)
{
        return iom.mouse_pipe;
}
