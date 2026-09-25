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

#ifndef IPC_CHANNEL_H
#define IPC_CHANNEL_H

#include <common/lock.h>
#include <object/thread.h>
#include <common/types.h>
#include <uapi/ipc.h>

/* declare that wait forever when receive msg */
#define OS_WAIT_FOREVER   0xFFFFFFFF

/* declare that no wait when receive msg */
#define OS_NO_WAIT        0

#define NOTIFY_MAX_LEN    152

#define CH_CNT_MAX   2
#define E_EX_TIMER_TIMEOUT 0xff

enum message_msgtype {
        MSG_TYPE_INVALID = 0,
        MSG_TYPE_NOTIF = 1,
        MSG_TYPE_CALL = 2,
};

struct src_msginfo {
        /* client's pid */
        u32 src_pid;
        /* client's tid */
        u32 src_tid;
        /* msg type(notification or call) */
        u16 msg_type;
};

/* client's ipc context */
struct client_msg_record {
        struct src_msginfo info;
        void *ksend_buf, *recv_buf;
        size_t send_len, recv_len;
        struct thread *client;
};

/* server's ipc context */
struct server_msg_record {
        void *recv_buf;
        size_t recv_len;
        struct thread *server;
        void *info;
};

struct msg_entry {
        struct list_head msg_queue_node;
        struct client_msg_record client_msg_record;
};

enum channel_state { CHANNEL_VALID, CHANNEL_INVALID };

struct channel {
        /* queue of waiting server threads */
        struct list_head thread_queue;
        /* queue of message */
        struct list_head msg_queue;
        struct lock lock;
        /* Only creater can receive at this channel */
        struct cap_group *creater;
        /* used to determine whether the channel is valid */
        int state;
};

/* full context of current ipc */
struct msg_hdl {
        struct list_head thread_queue_node;
        struct client_msg_record client_msg_record;
        struct server_msg_record server_msg_record;
        struct lock lock;
};

int close_channel(struct channel *channel, struct cap_group *cap_group);
void channel_deinit(void *ptr);
void msg_hdl_deinit(void *ptr);

cap_t ot_sys_tee_msg_create_msg_hdl(void);

cap_t ot_sys_tee_msg_create_channel(void);

int ot_sys_tee_msg_stop_channel(cap_t channel_cap);

int ot_sys_tee_msg_receive(cap_t channel_cap, void *recv_buf, size_t recv_len,
                           cap_t msg_hdl_cap, void *info, int timeout);

int ot_sys_tee_msg_call(cap_t channel_cap, void *send_buf, size_t send_len,
                        void *recv_buf, size_t recv_len,
                        struct timespec *timeout);
int ot_sys_tee_msg_reply(cap_t msg_hdl_cap, void *reply_buf, size_t reply_len);

int ot_sys_tee_msg_notify(cap_t channel_cap, void *send_buf, size_t send_len);

#endif /* IPC_CHANNEL_H */