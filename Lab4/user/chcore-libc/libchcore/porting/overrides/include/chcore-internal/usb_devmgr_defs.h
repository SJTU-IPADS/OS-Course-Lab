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

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_NAME_LEN 15

enum USB_DEVMGR_REQ {
        USB_DEVMGR_LIST_DEV = 1,
        USB_DEVMGR_OPEN,
        USB_DEVMGR_CLOSE,
        USB_DEVMGR_GET_DEV_SERVICE,
        // FIXME: GET_XXX_PIPE should not be placed at this layer
        USB_DEVMGR_GET_KEYBOARD_PIPE,
        USB_DEVMGR_GET_MOUSE_PIPE,
};

enum USB_DEV_CLASS {
        USB_UVC = 1,
        USB_SERIAL,
        USB_NIC,
        USB_KEYBORAD,
        USB_STORAGE,
        USB_MOUSE,
        USB_GAMEPAD,
        USB_OTHER,
};

enum UVC_REQ {
        UVC_CAPTURE_IMG,
};

struct uvc_request {
        enum UVC_REQ req;
        int shm_key;
};

enum SERIAL_REQ {
        SERIAL_WRITE = 1,
        SERIAL_READ,
        SERIAL_OPEN,
        SERIAL_CLOSE,
        SERIAL_SET_TERMIOS,
        SERIAL_GET_TERMIOS,
        SERIAL_TIOCMGET,
        SERIAL_TIOCMSET,
        SERIAL_DTR_RTS,
};

struct serial_request {
        enum SERIAL_REQ req;
        // used for send or receive data
        int datalen;
        int shmkey;
        // used for tiocmset
        int set;
        int clear;
        // userd for dtr_rts
        int on;
};

struct usb_devmgr_request {
        // request type
        enum USB_DEVMGR_REQ req;
        // specify dev type
        enum USB_DEV_CLASS dev_class;
        // dev specific request type
        union {
                struct uvc_request uvc_req;
                struct serial_request serial_req;
        };
        char devname[DEV_NAME_LEN];
        int dev_id;
        // used for open a device
        int wait;
};

struct dev_info {
        char devname[DEV_NAME_LEN];
        char vendor[DEV_NAME_LEN];
        enum USB_DEV_CLASS dev_class;
};

#ifdef __cplusplus
}
#endif
