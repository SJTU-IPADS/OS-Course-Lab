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
#include "serial_messenger.h"

extern __thread ipc_struct_t *devmgr_ipc_struct;

static int serial_write(struct dev_desc *dev_desc, void *data, int datalen);
static int serial_open(struct dev_desc *dev_desc);
static int serial_tiocmset(struct dev_desc *dev_desc,
                           struct serial_ctrl_data *ctrl_data);
static int serial_read(struct dev_desc *dev_desc, void *data, int datalen);

int serial_ctrl(struct dev_desc *dev_desc, u64 request, void *data, int len)
{
        int ret = 0;

        assert(dev_desc->dev_class == USB_SERIAL);

        switch (request) {
        case SERIAL_WRITE:
                /* code */
                ret = serial_write(dev_desc, data, len);
                break;
        case SERIAL_OPEN:
                ret = serial_open(dev_desc);
                break;
        case SERIAL_READ:
                ret = serial_read(dev_desc, data, len);
                break;
        case SERIAL_TIOCMSET:
                ret = serial_tiocmset(dev_desc,
                                      (struct serial_ctrl_data *)data);
                break;
        case SERIAL_TIOCMGET:
        case SERIAL_CLOSE:
        case SERIAL_SET_TERMIOS:
        case SERIAL_GET_TERMIOS:
        case SERIAL_DTR_RTS:
                printf("[Serial Messenger] Unsupported request type yet.\n");
                break;
        default:
                printf("[Serial Messenger] Unknown request type.\n");
                break;
        }

        return ret;
}

static int serial_write(struct dev_desc *dev_desc, void *data, int datalen)
{
        int ret = 0;
        ipc_msg_t *ipc_msg;
        struct usb_devmgr_request devmgr_request;
        int max_len;

        if (data == NULL && datalen == 0) {
                return 0;
        }

        devmgr_request.req = USB_DEVMGR_GET_DEV_SERVICE;
        devmgr_request.dev_class = USB_SERIAL;
        devmgr_request.dev_id = dev_desc->devid;
        devmgr_request.serial_req.req = SERIAL_WRITE;
        devmgr_request.serial_req.datalen = datalen;

        max_len = IPC_SHM_AVAILABLE - sizeof(devmgr_request);
        if (datalen > max_len) {
                // TODO: use share memory to support large data.
                // ipc_msg = ipc_create_msg(// 	devmgr_ipc_struct, sizeof(devmgr_request));
                // ipc_set_msg_data(
                // 	ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));
                return 0;
        } else {
                ipc_msg = ipc_create_msg(
                        devmgr_ipc_struct, sizeof(devmgr_request) + datalen);
                ipc_set_msg_data(
                        ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));
                ipc_set_msg_data(
                        ipc_msg, data, sizeof(devmgr_request), datalen);
        }

        ret = ipc_call(devmgr_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int serial_open(struct dev_desc *dev_desc)
{
        int ret = 0;
        ipc_msg_t *ipc_msg;
        struct usb_devmgr_request devmgr_request;

        devmgr_request.req = USB_DEVMGR_GET_DEV_SERVICE;
        devmgr_request.dev_class = USB_SERIAL;
        devmgr_request.dev_id = dev_desc->devid;
        devmgr_request.serial_req.req = SERIAL_OPEN;

        ipc_msg = ipc_create_msg(devmgr_ipc_struct, sizeof(devmgr_request));
        ipc_set_msg_data(ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));

        ret = ipc_call(devmgr_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int serial_read(struct dev_desc *dev_desc, void *data, int datalen)
{
        int ret = 0;
        int i;
        ipc_msg_t *ipc_msg;
        char *ipc_buff, *buff;
        struct usb_devmgr_request devmgr_request;
        int max_len;

        if (data == NULL && datalen == 0) {
                return 0;
        }

        devmgr_request.req = USB_DEVMGR_GET_DEV_SERVICE;
        devmgr_request.dev_class = USB_SERIAL;
        devmgr_request.dev_id = dev_desc->devid;
        devmgr_request.serial_req.req = SERIAL_READ;
        devmgr_request.serial_req.datalen = datalen;

        max_len = IPC_SHM_AVAILABLE - sizeof(devmgr_request);
        if (datalen > max_len) {
                // TODO: use share memory to support large data.
                // ipc_msg = ipc_create_msg(// 	devmgr_ipc_struct, sizeof(devmgr_request));
                // ipc_set_msg_data(
                // 	ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));
                return 0;
        } else {
                ipc_msg = ipc_create_msg(
                        devmgr_ipc_struct, sizeof(devmgr_request) + datalen);
                ipc_set_msg_data(
                        ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));
        }

        ret = ipc_call(devmgr_ipc_struct, ipc_msg);

        /* Copy data from ipc shared meomry to dest */
        ipc_buff = ipc_get_msg_data(ipc_msg) + sizeof(devmgr_request);
        buff = (char *)data;
        for (i = 0; i < ret && i < datalen; ++i) {
                buff[i] = ipc_buff[i];
        }

        ipc_destroy_msg(ipc_msg);
        return ret;
}

static int serial_tiocmset(struct dev_desc *dev_desc,
                           struct serial_ctrl_data *ctrl_data)
{
        int ret = 0;
        ipc_msg_t *ipc_msg;
        struct usb_devmgr_request devmgr_request;

        devmgr_request.req = USB_DEVMGR_GET_DEV_SERVICE;
        devmgr_request.dev_class = USB_SERIAL;
        devmgr_request.dev_id = dev_desc->devid;
        devmgr_request.serial_req.req = SERIAL_TIOCMSET;
        devmgr_request.serial_req.set = ctrl_data->set;
        devmgr_request.serial_req.clear = ctrl_data->clear;

        ipc_msg = ipc_create_msg(devmgr_ipc_struct, sizeof(devmgr_request));
        ipc_set_msg_data(ipc_msg, &devmgr_request, 0, sizeof(devmgr_request));

        ret = ipc_call(devmgr_ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        return ret;
}