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

#pragma once

#include <chcore-internal/usb_devmgr_defs.h>
#include <chcore/ipc.h>

#define DEV_NAME_LEN 15

/* Represents the device status. Stored in dev_desc.status */
#define DEV_STATUS_READY 1
#define DEV_STATUS_BUSY  2

/*
 * dev_desc: device descriptor
 * This struct contains some fields about a specific device like status
 * (maybe more in the future) and is typically not constructed by user.
 * dm_open will return a pointer of this type. User will use this pointer
 * as an argument in later operations like dm_ctrl or dm_close. Invoking
 * dm_close with a dev_desc will release the device and free this struct.
 */
struct dev_desc {
        int status;
        enum USB_DEV_CLASS dev_class;
        int devid;
        char devname[DEV_NAME_LEN];
};

typedef enum { SEARCH_DEV_NAME = 1, SEARCH_VENDOR } search_selector;

int dm_list_dev(struct dev_info **info);
struct dev_desc *dm_open(const char *devname, int wait);
int dm_ctrl(struct dev_desc *handle, u64 request, void *data, int len);
int dm_close(struct dev_desc *handle);
int dm_search_device(struct dev_info *dev_info, int num, const char *string,
                     search_selector selector);
