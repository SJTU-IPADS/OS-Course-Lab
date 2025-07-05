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

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_USER     2   /* A normal user-level thread */
#define TYPE_SHADOW   3   /* A shadow thread for serving IPC requests */
#define TYPE_REGISTER 4   /* A register cb thread for serving IPC registration */
#define TYPE_TRACEE   5   /* A traced thread for gdb server */

struct thread_args {
        /* specify the cap_group in which the new thread will be created */
        cap_t cap_group_cap;
        vaddr_t stack;
        vaddr_t pc;
        unsigned long arg;
        vaddr_t tls;
        unsigned int prio;
        unsigned int type;
        int *clear_child_tid;
};

#ifdef __cplusplus
}
#endif
