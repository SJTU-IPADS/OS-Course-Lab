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
#include <uapi/types.h>

// clang-format off
/**
 * This structure would be placed at the front of shared memory
 * of an IPC connection. So it should be 8 bytes aligned, letting
 * any kinds of data structures following it can be properly aligned.
 * 
 * This structure is written by IPC server, and read by client. 
 * IPC server should never read from it. 
 * 
 * Layout of shared memory is shown as follows:
 *           ┌───┬──────────────────────────────────────────────┐
 *           │   │                                              │
 *           │   │                                              │
 *           │   │     custom data(defined by IPC protocol)     │
 *           │   │                                              │
 *           │   │                                              │
 *           └─┬─┴──────────────────────────────────────────────┘
 *             │
 *             │
 *             ▼
 *   struct ipc_response_hdr
 */
// clang-format on
struct ipc_response_hdr {
    unsigned int return_cap_num;
} __attribute__((aligned(8)));

#define SHM_PTR_TO_CUSTOM_DATA_PTR(shm_ptr) ((void *)((char *)(shm_ptr) + sizeof(struct ipc_response_hdr)))

/**
 * @brief This type specifies the function signature that an IPC server 
 * should follow to be properly called by the kernel.
 * 
 * @param shm_ptr: pointer to start address of IPC shared memory. Use
 * SHM_PTR_TO_CUSTOM_DATA_PTR macro to convert it to concrete custom
 * data pointer.
 * @param max_data_len: length of IPC shared memory.
 * @param send_cap_num: number of capabilites sent by client in this request.
 * @param client_badge: badge of client.
 */
typedef void (*server_handler)(void *shm_ptr, unsigned int max_data_len, unsigned int send_cap_num, badge_t client_badge);
