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

#include <chcore/type.h>

enum GUI_REQ { GUI_CONN };

enum GUI_CLIENT_TYPE { GUI_CLIENT_CHCORE, GUI_CLIENT_WAYLAND };

/*
 * IPC request that GUI client sends to GUI server
 */
struct gui_request {
        enum GUI_REQ req;
        enum GUI_CLIENT_TYPE client_type;
};

typedef struct {
        u32 mods_depressed;
        u32 mods_latched;
        u32 mods_locked;
        u32 keys[6];
} raw_keyboard_event_t;

typedef struct {
        u32 buttons;
        int x;
        int y;
        int scroll;
} raw_pointer_event_t;
