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

#include <dev_messenger.h>
#include <chcore/ipc.h>
#include <chcore-internal/procmgr_defs.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <chcore/string.h>
#include <chcore/services.h>
#include "uvc_messenger.h"
#include "serial_messenger.h"

const char FromDM[] = "[Device messenger]:";

/*
 * Use a per-thread devmgr_ipc_struct so all the threads in a same process
 * can access devices simultaneously.
 */
__thread ipc_struct_t *devmgr_ipc_struct = NULL;
static int connect_to_devmgr();

/* Establish the connection to devmgr server */
static int connect_to_devmgr(void)
{
        int ret = 0;

        /* Check if the connection has been established */
        if (devmgr_ipc_struct) {
                ret = 0;
                goto out;
        }

        /* Try to connect the devmgr server */
        devmgr_ipc_struct = chcore_conn_srv(SERVER_USB_DEVMGR);
        if (devmgr_ipc_struct == NULL) {
                printf("%s Fail in connecting to devmgr server.\n", FromDM);
                ret = -ENXIO;
                goto out;
        }
out:
        return ret;
}

/*
 * Try to open a device specified by the devname.
 *
 * @devname: The name of the device. The devmgr server will look up this name
 *           in name service.
 * @wait:    Since multiple threads may want to use a device at the same time,
 *           so a lock will be used to protect the concurrent access. If the
 *           param wait equals one, this thread will wait until the device
 *           is released. Otherwise it will return -EBUSY.
 * @return:  This function returns a dev_desc pointer. Users can send control
 *           messages to the device using this dev_desc.
 */
struct dev_desc *dm_open(const char *devname, int wait)
{
        int ret, dev_id;
        int *data;
        ipc_msg_t *dev_ipc_msg;
        struct dev_desc *dev_desc;
        struct usb_devmgr_request request;
        enum USB_DEV_CLASS dev_class;

        /* Sanity check */
        if (!devname) {
                printf("%s Empty device name.\n", FromDM);
                goto fail;
        }

        /* Connect to devmgr server at the first time */
        if (devmgr_ipc_struct == NULL) {
                ret = connect_to_devmgr();
                if (ret) {
                        goto fail;
                }
        }

        /* prepare ipc parameters */
        request.req = USB_DEVMGR_OPEN;
        request.wait = wait;
        strlcpy(request.devname, devname, sizeof(request.devname));

        /* Prepare ipc_msg */
        dev_ipc_msg = ipc_create_msg(devmgr_ipc_struct, sizeof(request));
        ipc_set_msg_data(dev_ipc_msg, &request, 0, sizeof(request));

        ret = ipc_call(devmgr_ipc_struct, dev_ipc_msg);
        if (ret == -EBUSY) {
                printf("%s Fail in openning device %s: device busy.\n",
                       FromDM,
                       devname);
                ipc_destroy_msg(dev_ipc_msg);
                goto fail;

        } else if (ret == -ENODEV) {
                printf("%s Fail in openning device: unknown device name %s.\n",
                       FromDM,
                       devname);
                ipc_destroy_msg(dev_ipc_msg);
                goto fail;
        }

        /* We expect devmgr to return useful information. */
        data = (int *)ipc_get_msg_data(dev_ipc_msg);
        dev_class = *data;
        dev_id = *(data + 1);
        ipc_destroy_msg(dev_ipc_msg);

        dev_desc = malloc(sizeof(*dev_desc));
        dev_desc->status = DEV_STATUS_READY;
        dev_desc->dev_class = dev_class;
        dev_desc->devid = dev_id;
        return (struct dev_desc *)dev_desc;

fail:
        dev_desc = NULL;
        return dev_desc;
}

/*
 * Send a control message to the device.
 *
 * @dev_desc: Specify the target device.
 * @request:  Request type.
 * @data:     Data to be transmitted.
 * @len:      Data length.
 */
int dm_ctrl(struct dev_desc *dev_desc, u64 request, void *data, int len)
{
        int ret = 0;

        /* Check device descriptor */
        if (dev_desc == NULL) {
                printf("%s dev_desc is a null pointer.\n", FromDM);
                ret = -EINVAL;
                goto fail;
        }

        /* Check connection */
        if (devmgr_ipc_struct == NULL) {
                /* devmgr_ipc_struct cannot be NULL if any device has been
                 * opened */
                printf("%s Please open a device before sending message.\n",
                       FromDM);
                ret = -EINVAL;
                goto fail;
        }

        if (dev_desc->status != DEV_STATUS_READY) {
                printf("%s Device busy.\n", FromDM);
                ret = -EBUSY;
                goto fail;
        }

        dev_desc->status = DEV_STATUS_BUSY;

        switch (dev_desc->dev_class) {
        case USB_UVC:
                /* code */
                ret = uvc_ctrl(dev_desc, request, data, len);
                break;
        case USB_SERIAL:
                ret = serial_ctrl(dev_desc, request, data, len);
                break;
        default:
                break;
        }

        dev_desc->status = DEV_STATUS_READY;
fail:
        return ret;
}

/*
 * Release a device and related resources.
 */
int dm_close(struct dev_desc *dev_desc)
{
        int ret;
        ipc_msg_t *dev_ipc_msg;
        struct usb_devmgr_request request;

        /* Check connection */
        if (devmgr_ipc_struct == NULL) {
                printf("%s Trying to close a device without openning it.\n",
                       FromDM);
                return -EINVAL;
        }

        if (!dev_desc) {
                printf("%s Null pointer argument in %s.\n", FromDM, __func__);
                return -EINVAL;
        }

        /* Check device status */
        if (dev_desc->status != DEV_STATUS_READY) {
                printf("%s Cannot closing a device which is not ready.\n",
                       FromDM);
                return -EINVAL;
        }

        /* prepare ipc parameters */
        request.req = USB_DEVMGR_CLOSE;
        request.dev_id = dev_desc->devid;

        /* Prepare ipc_msg */
        dev_ipc_msg = ipc_create_msg(devmgr_ipc_struct, sizeof(request));
        ipc_set_msg_data(dev_ipc_msg, &request, 0, sizeof(request));

        ret = ipc_call(devmgr_ipc_struct, dev_ipc_msg);
        if (ret == -ENODEV) {
                printf("%s Try to close a unknown device.\n", FromDM);
                return ret;
        } else if (ret == -EACCES) {
                printf("%s Try to close a device which does not belong to you.\n",
                       FromDM);
                return ret;
        }
        ipc_destroy_msg(dev_ipc_msg);

        free(dev_desc);
        return ret;
}

/*
 * Get the information of all devices.
 *
 * @info:   Pass the address of a pointer. This function will help get the
 *          information of all devices and makes your pointer point to the
 *          buffer.
 * @return: return the num of devices
 */
int dm_list_dev(struct dev_info **info)
{
        int ret, i;
        u32 devnum;
        ipc_msg_t *dev_ipc_msg;
        struct dev_info *dev_info, *ret_info;
        struct usb_devmgr_request request;

        /* Check if the connection has been established */
        if (devmgr_ipc_struct == NULL) {
                ret = connect_to_devmgr();
                if (ret) {
                        goto out;
                }
        }

        /*
         * We rely on this ipc msg to get the information of devices.
         * But we can not know the length of the returned data because
         * the num of devices is unknown. We use a best guess 20 for now.
         */
        devnum = 20;
        dev_ipc_msg =
                ipc_create_msg(devmgr_ipc_struct, sizeof(u32) + devnum * sizeof(struct dev_info));

        /* Prepare parameters */
        request.req = USB_DEVMGR_LIST_DEV;
        ipc_set_msg_data(dev_ipc_msg, &request, 0, sizeof(request));
        ipc_set_msg_data(dev_ipc_msg, &devnum, sizeof(request), sizeof(u32));

        ret = ipc_call(devmgr_ipc_struct, dev_ipc_msg);
        if (ret == 0) {
                printf("%s Zero device found～ haha\n", FromDM);
                return 0;
        }

        if (ret > devnum) {
                // Guess wrong! Create a big enough ipc_msg this time.
                devnum = ret;
                dev_ipc_msg = ipc_create_msg(devmgr_ipc_struct, sizeof(u32) + devnum * sizeof(struct dev_info));

                request.req = USB_DEVMGR_LIST_DEV;
                ipc_set_msg_data(dev_ipc_msg, &request, 0, sizeof(request));
                ipc_set_msg_data(
                        dev_ipc_msg, &devnum, sizeof(request), sizeof(u32));

                ret = ipc_call(devmgr_ipc_struct, dev_ipc_msg);

                if (ret != devnum) {
                        printf("%s This should not happen since we do not support hot plugging for now.\n",
                               FromDM);
                        return -1;
                }
        }

        ret_info = (struct dev_info *)malloc(ret * sizeof(*dev_info));
        dev_info = (struct dev_info *)ipc_get_msg_data(dev_ipc_msg);
        for (i = 0; i < ret; ++i) {
                ret_info[i].dev_class = dev_info[i].dev_class;
                strncpy(ret_info[i].devname, dev_info[i].devname, DEV_NAME_LEN);
                strncpy(ret_info[i].vendor, dev_info[i].vendor, DEV_NAME_LEN);
        }
        ipc_destroy_msg(dev_ipc_msg);

        /* User should free this buffer himself later. */
        *info = ret_info;
out:
        return ret;
}

/*
 * Search a device with a given name or vendor
 */
int dm_search_device(struct dev_info *dev_info, int num, const char *string,
                     search_selector selector)
{
        int i;

        if (dev_info == NULL) {
                printf("%s Empty device info list.\n", FromDM);
                return -EINVAL;
        }

        switch (selector) {
        case SEARCH_DEV_NAME:
                for (i = 0; i < num; ++i) {
                        if (strncmp(dev_info[i].devname, string, DEV_NAME_LEN)
                            == 0) {
                                return 0;
                        }
                }
                break;
        case SEARCH_VENDOR:
                for (i = 0; i < num; ++i) {
                        if (strncmp(dev_info[i].vendor, string, DEV_NAME_LEN)
                            == 0) {
                                return 0;
                        }
                }
                break;
        default:
                printf("Unsupported serach selector.\n");
                return -EINVAL;
        }

        return -ENODEV;
}
