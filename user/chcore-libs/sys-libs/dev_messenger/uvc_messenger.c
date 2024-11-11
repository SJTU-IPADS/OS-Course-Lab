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
#include <chcore-internal/usb_devmgr_defs.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <dev_messenger.h>
#include "uvc_messenger.h"

extern __thread ipc_struct_t *devmgr_ipc_struct;
static int uvc_capture_image(struct dev_desc *dev_desc, void *data);

int uvc_ctrl(struct dev_desc *dev_desc, u64 request, void *data, int len)
{
        int ret = 0;

        assert(dev_desc->dev_class == USB_UVC);

        switch (request) {
        case UVC_CAPTURE_IMG:
                ret = uvc_capture_image(dev_desc, data);
                break;
        default:
                printf("[UVC Messenger] Unsupported request.");
                ret = -EINVAL;
                break;
        }

        return ret;
}

// Data should points to shm key.
static int uvc_capture_image(struct dev_desc *dev_desc, void *data)
{
        int ret;
        struct usb_devmgr_request devmgr_request;
        ipc_msg_t *ipc_msg;

        devmgr_request.req = USB_DEVMGR_GET_DEV_SERVICE;
        devmgr_request.dev_class = dev_desc->dev_class;
        devmgr_request.dev_id = dev_desc->devid;
        devmgr_request.uvc_req.req = UVC_CAPTURE_IMG;
        devmgr_request.uvc_req.shm_key = *(int *)data;

        assert(devmgr_ipc_struct != NULL);

        ipc_msg = ipc_create_msg(
                devmgr_ipc_struct, sizeof(struct usb_devmgr_request));
        ipc_set_msg_data(ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));
        ret = ipc_call(devmgr_ipc_struct, ipc_msg);

        if (ret < 0) {
                printf("[UVC Messenger] Error occurs when capturing image.\n");
                return ret;
        }

        ipc_destroy_msg(ipc_msg);
        return ret;
}
