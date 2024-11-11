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

#ifndef LIBIPC_IPC_LIB_H
#define LIBIPC_IPC_LIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* declare that wait forever when receive msg */
#define OS_WAIT_FOREVER 0xFFFFFFFF

/* declare that no wait when receiver msg */
#define OS_NO_WAIT 0

#define NOTIFY_MAX_LEN 152

#define CH_CNT_MAX         2
#define E_EX_TIMER_TIMEOUT 0xff

#define TID_MASK   0xFFFFULL
#define PID_OFFSET 16U
#define PID_MASK   ((0x1 << PID_OFFSET) - 1)

#define CID_OFFSET 12U
#if (CID_OFFSET >= PID_OFFSET)
#error "CID_OFFSET must be less than PID_OFFSET"
#endif

#define CID_BIT_MASK ((1U << (PID_OFFSET - CID_OFFSET)) - 1)
#if (CID_BIT_MASK < CH_CNT_MAX)
#error "CID_BIT_MASK must be larger than CH_CNT_MAX"
#endif

#define CID_MASK (CID_BIT_MASK << CID_OFFSET)

#define pid_to_taskid(tid, pid) \
        (((uint32_t)(tid) << PID_OFFSET) | ((uint32_t)(pid)&PID_MASK))
#define taskid_to_pid(task_id) ((task_id)&PID_MASK)
#define taskid_to_tid(task_id) (((task_id) >> PID_OFFSET) & TID_MASK)
#define cid_to_taskid(cid)     ((cid) & (~CID_MASK))
#define taskid_to_cid(task_id, ch_num) \
        ((task_id) | ((uint32_t)(ch_num) << CID_OFFSET))

typedef int cref_t;

typedef uint32_t taskid_t;

struct reg_items_st {
        bool reg_pid;
        bool reg_name;
        bool reg_tamgr;
};

struct src_msginfo {
        uint32_t src_pid;
        uint32_t src_tid;
        uint16_t msg_type;
};

enum message_msgtype {
        MSG_TYPE_INVALID = 0,
        MSG_TYPE_NOTIF = 1,
        MSG_TYPE_CALL = 2,
};

int32_t ipc_create_channel(const char *name, int32_t ch_cnt, cref_t *pp_ch[],
                           struct reg_items_st reg_items);

int32_t ipc_remove_channel(uint32_t task_id, const char *name, int32_t ch_num,
                           cref_t ch);

/* Get channel cap from PathMgr based on name. */
int32_t ipc_get_ch_from_path(const char *name, cref_t *p_ch);

/* Get channel cap based on task_id. */
int32_t ipc_get_ch_from_taskid(uint32_t task_id, int32_t ch_num, cref_t *p_ch);

/* Get task_id of the server based on the registered TAMgr information. */
uint32_t ipc_hunt_by_name(const char *name, uint32_t *task_id);

/* Obtain the associated channel of the current thread tls. */
int32_t ipc_get_my_channel(int32_t ch_num, cref_t *p_ch);

/* Use with ipc_get_ch_from_path to release resources on the client. */
uint32_t ipc_release_from_path(const char *name, cref_t rref);

/* Use with ipc_get_ch_from_taskid to release resources on the client and clear
 * the cache. */
uint32_t ipc_release_from_taskid(uint32_t task_id, int32_t ch_num);

/* Use with ipc_hunt_by_name to clear the cache. */
uint32_t ipc_release_by_name(const char *name);

int32_t ipc_msg_call(cref_t channel, void *send_buf, size_t send_len,
                     void *reply_buf, size_t reply_len, int32_t timeout);

int32_t ipc_msg_notification(cref_t ch, void *send_buf, size_t send_len);

int32_t ipc_msg_reply(cref_t msg_hdl, void *reply_buf, size_t len);

int32_t ipc_msg_receive(cref_t channel, void *recv_buf, size_t recv_len,
                        cref_t msg_hdl, struct src_msginfo *info,
                        int32_t timeout);

cref_t ipc_msg_create_hdl(void);

int32_t ipc_msg_delete_hdl(cref_t msg_hdl);

int32_t ipc_save_my_msghdl(cref_t msg_hdl);

cref_t ipc_get_my_msghdl(void);

uint32_t get_self_taskid(void);

bool check_ref_valid(cref_t cref);

#endif
