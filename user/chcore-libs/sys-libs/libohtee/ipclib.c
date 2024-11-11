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

#include <chcore/syscall.h>
#include <chcore/opentrustee/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <chcore-internal/chanmgr_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/ipc.h>
#include <chcore/container/hashtable.h>
#include <pthread.h>

#include <ipclib.h>

#define ipc_info(fmt, ...) \
        printf("%s %d: " fmt, __func__, __LINE__, ##__VA_ARGS__)

struct __ipc_tls_st {
        ipc_struct_t *chanmgr_ipc_struct;
        int channel[2];
        int msg_hdl;
};
static struct __ipc_tls_st __ipc_tls__[1024];

#define __ipc_tls (__ipc_tls__[gettid()])

/* attain chanmgr ipc_struct */
static ipc_struct_t *__conn_server(void)
{
        cap_t server_cap;
        ipc_msg_t *ipc_msg;
        struct proc_request *req;
        ipc_struct_t *cis = NULL;
        int ret;

        ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        req->req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(req->get_service_cap.service_name, SERVER_CHANMGR,
                SERVICE_NAME_LEN);
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);
        if (ret != 0) {
                goto out;
        }

        server_cap = ipc_get_msg_cap(ipc_msg, 0);
        if (server_cap < 0) {
                ret = -1;
                goto out;
        }

        do {
                cis = ipc_register_client(server_cap);
                usys_yield();
        } while (cis == NULL);

out:
        ipc_destroy_msg(ipc_msg);
        return cis;
}

/* per-thread chanmgr ipc_struct */
static ipc_struct_t *__chanmgr_ipc_struct(void)
{
        if (__ipc_tls.chanmgr_ipc_struct == NULL) {
                __ipc_tls.chanmgr_ipc_struct = __conn_server();
        }

        return __ipc_tls.chanmgr_ipc_struct;
}

#define chanmgr_ipc_struct (__chanmgr_ipc_struct())

static int __ipc_util_create_channel(const char *name, int ch_num, int channel,
                                     struct reg_items_st reg_items)
{
        ipc_msg_t *ipc_msg;
        struct chan_request *req;
        int ret;

        ipc_msg = ipc_create_msg_with_cap(
                chanmgr_ipc_struct, sizeof(struct chan_request), 1);
        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);
        req->req = CHAN_REQ_CREATE_CHANNEL;
        ret = ipc_set_msg_cap(ipc_msg, 0, channel);
        if (ret < 0) {
                ret = -EINVAL;
                goto out_fail;
        }

        memset(req->create_channel.name, 0, sizeof(req->create_channel.name));
        if (name != NULL && (reg_items.reg_name || reg_items.reg_tamgr)) {
                if (strlen(name) >= CHAN_REQ_NAME_LEN) {
                        ret = -ENAMETOOLONG;
                        goto out_fail;
                }
                memcpy(req->create_channel.name, name, strlen(name));
        }
        if (reg_items.reg_pid) {
                req->create_channel.ch_num = ch_num;
        }
        memcpy(&req->create_channel.reg_items,
               &reg_items,
               sizeof(struct reg_items_st));

        ret = ipc_call(chanmgr_ipc_struct, ipc_msg);

out_fail:
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int __ipc_util_remove_channel(const char *name, int ch_num,
                                     uint32_t taskid)
{
        ipc_msg_t *ipc_msg;
        struct chan_request *req;
        int ret;

        if (taskid == 0) {
                taskid = get_self_taskid();
        }

        ipc_msg =
                ipc_create_msg(chanmgr_ipc_struct, sizeof(struct chan_request));
        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);
        req->req = CHAN_REQ_REMOVE_CHANNEL;

        memset(req->remove_channel.name, 0, sizeof(req->remove_channel.name));
        if (name) {
                if (strlen(name) >= CHAN_REQ_NAME_LEN) {
                        ret = -ENAMETOOLONG;
                        goto out_fail;
                }
                memcpy(req->remove_channel.name, name, strlen(name));
        }
        if (ch_num == 0 || ch_num == 1) {
                req->remove_channel.taskid = taskid;
                req->remove_channel.ch_num = ch_num;
        }

        ret = ipc_call(chanmgr_ipc_struct, ipc_msg);

out_fail:
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int __ipc_util_hunt_by_name(const char *name, uint32_t *taskid)
{
        ipc_msg_t *ipc_msg;
        struct chan_request *req;
        int ret;

        ipc_msg =
                ipc_create_msg(chanmgr_ipc_struct, sizeof(struct chan_request));
        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);
        req->req = CHAN_REQ_HUNT_BY_NAME;

        if (name == NULL) {
                ret = -EINVAL;
                goto out_fail;
        }
        if (strlen(name) >= CHAN_REQ_NAME_LEN) {
                ret = -ENAMETOOLONG;
                goto out_fail;
        }

        memset(req->hunt_by_name.name, 0, sizeof(req->hunt_by_name.name));
        memcpy(req->hunt_by_name.name, name, strlen(name));
        ret = ipc_call(chanmgr_ipc_struct, ipc_msg);
        if (ret == 0) {
                *taskid = req->hunt_by_name.taskid;
        }

out_fail:
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int __ipc_util_get_ch_from_path(const char *name, int *cap)
{
        ipc_msg_t *ipc_msg;
        struct chan_request *req;
        int ret;

        ipc_msg =
                ipc_create_msg(chanmgr_ipc_struct, sizeof(struct chan_request));
        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);
        req->req = CHAN_REQ_GET_CH_FROM_PATH;

        if (name == NULL) {
                ret = -EINVAL;
                goto out_fail;
        }
        if (strlen(name) >= CHAN_REQ_NAME_LEN) {
                ret = -ENAMETOOLONG;
                goto out_fail;
        }

        memset(req->get_ch_from_path.name,
               0,
               sizeof(req->get_ch_from_path.name));
        memcpy(req->get_ch_from_path.name, name, strlen(name));
        ret = ipc_call(chanmgr_ipc_struct, ipc_msg);
        if (ret == 0) {
                ret = ipc_get_msg_cap(ipc_msg, 0);
                if (ret < 0) {
                        ret = -EINVAL;
                } else {
                        *cap = ret;
                        ret = 0;
                }
        }

out_fail:
        ipc_destroy_msg(ipc_msg);

        return ret;
}

static int __ipc_util_get_ch_from_taskid(uint32_t taskid, int ch_num, int *cap)
{
        ipc_msg_t *ipc_msg;
        struct chan_request *req;
        int ret;

        ipc_msg =
                ipc_create_msg(chanmgr_ipc_struct, sizeof(struct chan_request));
        req = (struct chan_request *)ipc_get_msg_data(ipc_msg);
        req->req = CHAN_REQ_GET_CH_FROM_TASKID;

        req->get_ch_from_taskid.taskid = taskid;
        req->get_ch_from_taskid.ch_num = ch_num;
        ret = ipc_call(chanmgr_ipc_struct, ipc_msg);
        if (ret == 0) {
                ret = ipc_get_msg_cap(ipc_msg, 0);
                if (ret < 0) {
                        ret = -EINVAL;
                } else {
                        *cap = ret;
                        ret = 0;
                }
        }

        ipc_destroy_msg(ipc_msg);

        return ret;
}

#define HTABLE_SIZE 1024

struct task2capmgr {
        pthread_mutex_t lock;
        struct htable table;
        volatile int initialized;
};

static struct task2capmgr *__task2cap(void)
{
        static struct task2capmgr task2cap_table = {.initialized = 0};
        int old;

        old = __sync_val_compare_and_swap(&task2cap_table.initialized, 0, 1);
        if (old == 0) {
                init_htable(&task2cap_table.table, HTABLE_SIZE);
                pthread_mutex_init(&task2cap_table.lock, NULL);
        }
        return &task2cap_table;
}

#define task2cap (__task2cap())

struct task2cap_ent {
        struct hlist_node task2cap_node;
        cap_t cap;
        taskid_t taskid;
        int ch_num;
};

static struct task2cap_ent *__get_entry(taskid_t taskid, int ch_num)
{
        struct task2cap_ent *entry;
        struct hlist_head *bucket;

        bucket = htable_get_bucket(&task2cap->table, taskid);
        for_each_in_hlist (entry, task2cap_node, bucket) {
                if (entry->taskid == taskid && entry->ch_num == ch_num) {
                        return entry;
                }
        }

        return NULL;
}

/*
 * Create channel(s) and register mapping info among name, taskid and channel
 * @name: channel's name
 * @ch_cnt: the number(1 or 2) of channel(s) to be created
 * @pp_ch: output the cap(s) of corresponding channel(s)
 * @reg_items: guidance on specific registration behaviors
 *
 * Create ch_cnt channel(s).
 * if reg_items.reg_name is true and name is not NULL:
 *      register (name, channel[0]) mapping info into chanmgr
 * if reg_items.reg_tamgr is true and name is not NULL:
 *      register (name, taskid) mapping info into chanmgr
 * if reg_items.reg_pid is true:
 *      register ((taskid, ch_num), channel) mapping info into chanmgr
 *      save channel cap(s) into pthread struct
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_create_channel(const char *name, int32_t ch_cnt, cref_t *pp_ch[],
                           struct reg_items_st reg_items)
{
        int ret, i;
        cap_t chan[2];

        if (!(ch_cnt > 0 && ch_cnt <= CH_CNT_MAX)) {
                ret = -EINVAL;
                goto out_fail;
        }

        for (i = 0; i < ch_cnt; i++) {
                ret = usys_tee_msg_create_channel();
                if (ret < 0) {
                        goto out_free_channel;
                }
                chan[i] = ret;
        }

        /* remove old local channels if reg_pid */
        if (reg_items.reg_pid && __ipc_tls.channel[0] != 0) {
                ipc_remove_channel(0, NULL, 0, 0);
        }
        ret = __ipc_util_create_channel(name, 0, chan[0], reg_items);
        if (ret < 0) {
                goto out_free_channel;
        }
        if (ch_cnt == CH_CNT_MAX) {
                if (reg_items.reg_pid && __ipc_tls.channel[1] != 0) {
                        ipc_remove_channel(0, NULL, 1, 0);
                }
                ret = __ipc_util_create_channel(NULL, 1, chan[1], reg_items);
                if (ret < 0) {
                        goto out_remove_first_channel;
                }
        }

        for (i = 0; i < ch_cnt; i++) {
                if (reg_items.reg_pid) {
                        __ipc_tls.channel[i] = chan[i];
                }
                if (pp_ch) {
                        *pp_ch[i] = chan[i];
                }
        }

        return 0;

out_remove_first_channel:
        __ipc_util_remove_channel(name, 0, 0);

out_free_channel:
        for (i = i - 1; i >= 0; i--) {
                usys_revoke_cap(chan[i], false);
        }

out_fail:
        return ret;
}

/*
 * Remove channel(s) and deregister mapping info among name, taskid and channel
 * @task_id: taskid in ((taskid, ch_num), channel) mapping info
 * @name: name in (name, channel) & (name, taskid) mapping info
 * @ch_num: ch_num in ((taskid, ch_num), channel) mapping info & which channel
 * in pthread struct to be removed
 * @ch: channel cap
 *
 * if name is not NULL:
 *      deregister (name, channel) and (name, taskid) mapping info in chanmgr if
 * exist deregister ((taskid, ch_num), channel) mapping info in chanmgr if exist
 * if ch is 0, remove the ch_numth channel in pthread struct
 * else, remove ch
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_remove_channel(uint32_t task_id, const char *name, int32_t ch_num,
                           cref_t ch)
{
        int ret;

        if (task_id == 0) {
                task_id = get_self_taskid();
        }

        if (ch == 0) {
                if (ch_num == 0 || ch_num == 1) {
                        ch = __ipc_tls.channel[ch_num];
                } else {
                        return -EINVAL;
                }
        }

        ret = __ipc_util_remove_channel(name, ch_num, task_id);
        if (ret != 0) {
                goto out;
        }

        ret = usys_tee_msg_stop_channel(ch);
        if (ret != 0) {
                goto out;
        }

        if ((ch_num == 0 || ch_num == 1) && task_id == get_self_taskid()) {
                __ipc_tls.channel[ch_num] = 0;
        }

        ret = usys_revoke_cap(ch, false);

out:
        return ret;
}

/*
 * Release channel reference cap from path.
 * @path: not used in ChCore
 * @rref: channel cap
 *
 * Revoke the channel cap directly.
 * Return:
 *      0: success
 *      errno: fail
 */
uint32_t ipc_release_from_path(const char *path, cref_t rref)
{
        int ret;

        ret = usys_revoke_cap(rref, false);

        return ret;
}

/*
 * Release channel reference cap from task_id.
 *
 * Revoke the corresponding channel cap.
 * Return:
 *      0: success
 *      errno: fail
 */
uint32_t ipc_release_from_taskid(uint32_t task_id, int32_t ch_num)
{
        int ret = 0;
        struct task2cap_ent *entry;

        pthread_mutex_lock(&task2cap->lock);

        entry = __get_entry(task_id, ch_num);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        usys_revoke_cap(entry->cap, false);
        htable_del(&entry->task2cap_node);
        free(entry);

out:
        pthread_mutex_unlock(&task2cap->lock);
        return ret;
}

uint32_t ipc_release_by_name(const char *name)
{
        return 0;
}

/*
 * Get channel cap from chanmgr, which can be indexed through channel name.
 * @name: name of the channel
 * @p_ch: output the cap of the channel
 *
 * Send ChCore IPC request to chanmgr to get channel cap.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_get_ch_from_path(const char *name, cref_t *p_ch)
{
        int ret;

        ret = __ipc_util_get_ch_from_path(name, p_ch);

        return ret;
}

/*
 * Get channel cap from chanmgr, which can be indexed through (taskid, ch_num).
 * @taskid: taskid in ((taskid, ch_num), channel) mapping info
 * @ch_num: ch_num in ((taskid, ch_num), channel) mapping info
 * @p_ch: output the cap of the channel
 *
 * Send ChCore IPC request to chanmgr to get channel cap.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_get_ch_from_taskid(uint32_t taskid, int32_t ch_num, cref_t *p_ch)
{
        int ret;
        struct task2cap_ent *entry;

        pthread_mutex_lock(&task2cap->lock);

        entry = __get_entry(taskid, ch_num);
        if (entry != NULL) {
                *p_ch = entry->cap;
                ret = 0;
                goto out;
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out;
        }

        ret = __ipc_util_get_ch_from_taskid(taskid, ch_num, p_ch);
        if (ret != 0) {
                goto out_free_entry;
        }

        entry->taskid = taskid;
        entry->ch_num = ch_num;
        entry->cap = *p_ch;
        init_hlist_node(&entry->task2cap_node);
        htable_add(&task2cap->table, taskid, &entry->task2cap_node);

        pthread_mutex_unlock(&task2cap->lock);
        return 0;

out_free_entry:
        free(entry);
out:
        pthread_mutex_unlock(&task2cap->lock);
        return ret;
}

/*
 * Get server's taskid from chanmgr, which can be indexed through name.
 * @name: name of the channel
 * @task_id: output the taskid of the server who created the channel named @name
 *
 * Send ChCore IPC request to chanmgr to get taskid.
 * Return:
 *      0: success
 *      errno: fail
 */
uint32_t ipc_hunt_by_name(const char *name, uint32_t *task_id)
{
        int ret;

        ret = __ipc_util_hunt_by_name(name, task_id);

        return ret;
}

/*
 * Get the ch_numth channel cap stored in pthread struct.
 * @ch_num: the ch_numth channel in pthread struct
 * @p_ch: output the ch_numth channel cap in pthread struct
 *
 * Get the ch_numth channel cap from pthread struct.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_get_my_channel(int32_t ch_num, cref_t *p_ch)
{
        if (ch_num != 0 && ch_num != 1) {
                return -EINVAL;
        }
        if (__ipc_tls.channel[ch_num] <= 0) {
                return -ENOENT;
        }
        *p_ch = __ipc_tls.channel[ch_num];
        return 0;
}

/*
 * ipc_msg_call to the given channel, waiting for server's ipc_reply.
 * @channel: cap of the channel
 * @send_buf: send message buffer to be transfered to the server
 * @send_len: length of the send buffer
 * @reply_buf: reply buffer to be filled in when ipc_msg_reply from the server
 * @reply_len: length of reply buffer
 * @timeout: timeout
 *
 * ipc_msg_call will block current thread temporarily and transfer msg @send_buf
 * to server. Current thread will be resumed after the server calls
 * ipc_msg_reply.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_msg_call(cref_t channel, void *send_buf, size_t send_len,
                     void *reply_buf, size_t reply_len, int32_t timeout)
{
        int ret;

        ret = usys_tee_msg_call(
                channel, send_buf, send_len, reply_buf, reply_len, NULL);

        return ret;
}

/*
 * ipc_msg_reply to the given msg_hdl.
 * @msg_hdl: cap of the msg_hdl
 * @reply_buf: message buffer to be transfered back to client
 * @len: length of reply buffer
 * @timeout: timeout
 *
 * The client to be replied, which is recored in msg_hdl, will be resumed after
 * ipc_msg_reply.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_msg_reply(cref_t msg_hdl, void *reply_buf, size_t len)
{
        int ret;

        ret = usys_tee_msg_reply(msg_hdl, reply_buf, len);

        return ret;
}

/*
 * ipc_msg_notification to the given channel.
 * @ch: cap of the channel
 * @send_buf: message buffer to be transfered to the server
 * @send_len: length of the send buffer
 *
 * ipc_msg_notification sends message asynchronously to the given channel, which
 * will not block current thread at all.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_msg_notification(cref_t ch, void *send_buf, size_t send_len)
{
        int ret;

        ret = usys_tee_msg_notify(ch, send_buf, send_len);

        return ret;
}

/*
 * ipc_msg_receive at the given channel.
 * @channel: cap of the channel
 * @recv_buf: the buffer to be filled in with client's message
 * @recv_len: length of the recv buffer
 * @msg_hdl: cap of the msg_hdl
 * @info: the info(client's taskid, message type(sync or async)) of the ipc
 * @timeout: timeout
 *
 * Current server thread will wait for message.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_msg_receive(cref_t channel, void *recv_buf, size_t recv_len,
                        cref_t msg_hdl, struct src_msginfo *info,
                        int32_t timeout)
{
        int ret;

        ret = usys_tee_msg_receive(
                channel, recv_buf, recv_len, msg_hdl, info, timeout);

        return ret;
}

/*
 * Create msg_hdl.
 *
 * Create msg_hdl and return cap of the msg_hdl.
 * Return:
 *      >= 0: cap of the msg_hdl
 *      errno: fail
 */
cref_t ipc_msg_create_hdl(void)
{
        int ret;

        ret = usys_tee_msg_create_msg_hdl();

        return ret;
}

/*
 * Delete msg_hdl.
 * @msg_hdl: cap of the msg_hdl
 *
 * Revoke msg_hdl cap directly.
 * Return:
 *      0: success
 *      errno: fail
 */
int32_t ipc_msg_delete_hdl(cref_t msg_hdl)
{
        int ret;

        ret = usys_revoke_cap(msg_hdl, false);

        return ret;
}

/*
 * Save msg_hdl locally.
 * @msg_hdl: cap of the msg_hdl
 *
 * Save msg_hdl cap into pthread struct.
 */
int32_t ipc_save_my_msghdl(cref_t msg_hdl)
{
        __ipc_tls.msg_hdl = msg_hdl;
        return 0;
}

/*
 * Get msg_hdl locally.
 *
 * Return msg_hdl cap stored in pthread struct.
 * Return:
 *      >= 0: cap of the msg_hdl
 *      errno: fail
 */
cref_t ipc_get_my_msghdl(void)
{
        if (__ipc_tls.msg_hdl == 0) {
                __ipc_tls.msg_hdl = ipc_msg_create_hdl();
        }
        return __ipc_tls.msg_hdl;
}

/*
 * Get taskid of current thread.
 *
 * Return taskid of current thread.
 * Return: taskid
 */
uint32_t get_self_taskid(void)
{
        return pid_to_taskid(gettid(), getpid());
}

bool check_ref_valid(cref_t cref)
{
        return cref >= 0;
}
