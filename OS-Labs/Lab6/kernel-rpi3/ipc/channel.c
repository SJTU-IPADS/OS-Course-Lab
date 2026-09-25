/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <ipc/channel.h>
#include <sched/sched.h>
#include <mm/uaccess.h>
#include <common/util.h>

#define GTASK_PID (7)
#define GTASK_TID (0xa)

/*
 * 1. if msg queue is empty, server thread will be inserted into thread queue
 *    and not be sched until client thread enqueue it
 * 2. if not, server thread will directly consume the msg and return
 */
static int __tee_msg_receive(struct channel *channel, void *recv_buf,
                             size_t recv_len, struct msg_hdl *msg_hdl,
                             void *info, int timeout)
{
        struct msg_entry *msg_entry;
        size_t copy_len;
        int ret;

        if (channel->creater != current_cap_group) {
                return -EINVAL;
        }

        lock(&channel->lock);
        lock(&msg_hdl->lock);

        if (channel->state == CHANNEL_INVALID) {
                unlock(&msg_hdl->lock);
                unlock(&channel->lock);
                return -EINVAL;
        }

        if (list_empty(&channel->msg_queue)) {
                kdebug("%s: list_empty(&channel->msg_queue)\n", __func__);
                if (timeout == OS_NO_WAIT) {
                        unlock(&msg_hdl->lock);
                        unlock(&channel->lock);
                        return E_EX_TIMER_TIMEOUT;
                }

                msg_hdl->server_msg_record.server = current_thread;
                msg_hdl->server_msg_record.recv_buf = recv_buf;
                msg_hdl->server_msg_record.recv_len = recv_len;
                msg_hdl->server_msg_record.info = info;

                list_append(&msg_hdl->thread_queue_node,
                            &channel->thread_queue);

                unlock(&msg_hdl->lock);
                unlock(&channel->lock);

                /* obj_put due to noreturn */
                obj_put(msg_hdl);
                obj_put(channel);

                thread_set_ts_blocking(current_thread);
                sched();
                eret_to_thread(switch_context());
                BUG_ON(1);
        } else {
                kdebug("%s: !list_empty(&channel->msg_queue)\n", __func__);
                msg_entry = list_entry(channel->msg_queue.next,
                                       struct msg_entry,
                                       msg_queue_node);
                copy_len = MIN(msg_entry->client_msg_record.send_len, recv_len);
                ret = copy_to_user(recv_buf,
                                   msg_entry->client_msg_record.ksend_buf,
                                   copy_len);
                if (ret < 0) {
                        ret = -EFAULT;
                        goto out_unlock;
                }

                ret = copy_to_user(info,
                                   (char *)&msg_entry->client_msg_record.info,
                                   sizeof(struct src_msginfo));
                if (ret < 0) {
                        ret = -EFAULT;
                        goto out_unlock;
                }
                list_del(&msg_entry->msg_queue_node);

                memcpy(&msg_hdl->client_msg_record,
                       &msg_entry->client_msg_record,
                       sizeof(struct client_msg_record));
                ret = 0;

                kfree(msg_entry->client_msg_record.ksend_buf);
                kfree(msg_entry);

        out_unlock:
                unlock(&msg_hdl->lock);
                unlock(&channel->lock);
                return ret;
        }
}

/*
 * Client calling __tee_msg_send will cause
 * 1. if thread queue is empty, client thread's msg will be inserted
 *    into msg queue
 * 2. if not, client thread will directly choose and sched_enqueue one
 *    server thread
 */
static int __tee_msg_send(struct channel *channel,
                          struct client_msg_record *client_msg_record)
{
        struct msg_hdl *msg_hdl;
        struct msg_entry *msg_entry;
        struct thread *client, *server;
        size_t copy_len;
        int ret = 0;

        lock(&channel->lock);

        if (channel->state == CHANNEL_INVALID) {
                ret = -EINVAL;
                goto out;
        }

        if (list_empty(&channel->thread_queue)) {
                kdebug("%s: list_empty(&channel->thread_queue)\n", __func__);
                msg_entry = kmalloc(sizeof(*msg_entry));

                memcpy(&msg_entry->client_msg_record,
                       client_msg_record,
                       sizeof(*client_msg_record));

                list_append(&msg_entry->msg_queue_node, &channel->msg_queue);
        } else {
                kdebug("%s: !list_empty(&channel->thread_queue)\n", __func__);
                msg_hdl = list_entry(channel->thread_queue.next,
                                     struct msg_hdl,
                                     thread_queue_node);

                lock(&msg_hdl->lock);
                list_del(&msg_hdl->thread_queue_node);

                memcpy(&msg_hdl->client_msg_record,
                       client_msg_record,
                       sizeof(*client_msg_record));

                server = msg_hdl->server_msg_record.server;
                client = current_thread;

                current_thread = server;
                switch_thread_vmspace_to(server);

                copy_len = MIN(client_msg_record->send_len,
                               msg_hdl->server_msg_record.recv_len);
                ret = copy_to_user(msg_hdl->server_msg_record.recv_buf,
                                   client_msg_record->ksend_buf,
                                   copy_len);
                if (ret < 0) {
                        goto out_copy;
                }

                ret = copy_to_user(msg_hdl->server_msg_record.info,
                                   (char *)&msg_hdl->client_msg_record.info,
                                   sizeof(struct src_msginfo));

        out_copy:
                current_thread = client;
                switch_thread_vmspace_to(client);
                if (ret < 0) {
                        ret = -EFAULT;
                        goto out_unlock;
                }

                kfree(client_msg_record->ksend_buf);
                arch_set_thread_return(server, 0);
                BUG_ON(sched_enqueue(server));
                kdebug("%s: enqueued %s\n",
                       __func__,
                       server->cap_group->cap_group_name);
        out_unlock:
                unlock(&msg_hdl->lock);
        }

out:
        unlock(&channel->lock);
        return ret;
}

/*
 * After client calling __tee_msg_send, client thread will be record in
 * msg_hdl and not be sched until server thread __tee_msg_reply to enqueue it
 */
static int __tee_msg_call(struct channel *channel, void *send_buf,
                          size_t send_len, void *recv_buf, size_t recv_len,
                          struct timespec *timeout)
{
        struct client_msg_record client_msg_record;
        void *ksend_buf;
        int ret;

        kdebug("%s: %s calls %s\n",
               __func__,
               current_cap_group->cap_group_name,
               channel->creater->cap_group_name);

        if ((ksend_buf = kmalloc(send_len)) == NULL) {
                ret = -ENOMEM;
                goto out_fail;
        }
        ret = copy_from_user(ksend_buf, send_buf, send_len);
        if (ret < 0) {
                ret = -EFAULT;
                goto out_free_ksend_buf;
        }

        client_msg_record.client = current_thread;
        client_msg_record.ksend_buf = ksend_buf;
        client_msg_record.send_len = send_len;
        client_msg_record.recv_buf = recv_buf;
        client_msg_record.recv_len = recv_len;
        client_msg_record.info.msg_type = MSG_TYPE_CALL;
        client_msg_record.info.src_pid = current_cap_group->pid;
        client_msg_record.info.src_tid = current_thread->cap;
        /*
         * There is some code in the framework that assumes the taskid of gtask
         * is 0. Kernel should do this adaption work.
         */
        if (current_cap_group->pid == GTASK_PID
            && current_thread->cap == GTASK_TID) {
                client_msg_record.info.src_pid = 0;
                client_msg_record.info.src_tid = 0;
        }

        ret = __tee_msg_send(channel, &client_msg_record);
        if (ret != 0) {
                goto out_free_ksend_buf;
        }

        /* obj_put due to noreturn */
        obj_put(channel);

        thread_set_ts_blocking(current_thread);
        sched();
        eret_to_thread(switch_context());
        BUG_ON(1);

out_free_ksend_buf:
        kfree(ksend_buf);

out_fail:
        return ret;
}

/* Enqueue blocking client thread */
static int __tee_msg_reply(struct msg_hdl *msg_hdl, void *reply_buf,
                           size_t reply_len)
{
        struct thread *client, *server;
        void *kreply_buf;
        size_t copy_len;
        int ret = 0;

        kdebug("%s: %s replies to %s\n",
               __func__,
               current_cap_group->cap_group_name,
               msg_hdl->client_msg_record.client->cap_group->cap_group_name);

        lock(&msg_hdl->lock);

        if (msg_hdl->client_msg_record.info.msg_type != MSG_TYPE_CALL) {
                ret = -EINVAL;
                goto out;
        }

        if ((kreply_buf = kmalloc(reply_len)) == NULL) {
                ret = -ENOMEM;
                goto out;
        }
        ret = copy_from_user(kreply_buf, reply_buf, reply_len);
        if (ret < 0) {
                ret = -EFAULT;
                goto out_free_kreply_buf;
        }

        client = msg_hdl->client_msg_record.client;
        server = current_thread;

        current_thread = client;
        switch_thread_vmspace_to(client);

        copy_len = MIN(reply_len, msg_hdl->client_msg_record.recv_len);
        ret = copy_to_user(
                msg_hdl->client_msg_record.recv_buf, kreply_buf, copy_len);

        current_thread = server;
        switch_thread_vmspace_to(server);
        if (ret < 0) {
                ret = -EFAULT;
                goto out_free_kreply_buf;
        }

        /*
         * Wait for client's kernel stack to make sure that
         * sched_enqueue(client) executes after sched() in client's
         * __tee_msg_call. Note that wait_for_kernel_stack executes with
         * server's msg_hdl locked. Client should NOT hold the lock of msg_hdl,
         * which is established under the assumption that msg_hdl cap should not
         * be distributed to others.
         */
        wait_for_kernel_stack(client);

        arch_set_thread_return(client, 0);
        BUG_ON(sched_enqueue(client));

        /* A call cannot be replied successfully twice. */
        msg_hdl->client_msg_record.info.msg_type = MSG_TYPE_INVALID;

out_free_kreply_buf:
        kfree(kreply_buf);
out:
        unlock(&msg_hdl->lock);
        return ret;
}

/* __tee_msg_send and return directly */
static int __tee_msg_notify(struct channel *channel, void *send_buf,
                            size_t send_len)
{
        struct client_msg_record client_msg_record;
        void *ksend_buf;
        int ret;

        kdebug("%s: %s notifies %s\n",
               __func__,
               current_cap_group->cap_group_name,
               channel->creater->cap_group_name);

        if ((ksend_buf = kmalloc(send_len)) == NULL) {
                ret = -ENOMEM;
                goto out_fail;
        }
        ret = copy_from_user(ksend_buf, send_buf, send_len);
        if (ret < 0) {
                ret = -EFAULT;
                goto out_free_ksend_buf;
        }

        client_msg_record.client = current_thread;
        client_msg_record.ksend_buf = ksend_buf;
        client_msg_record.send_len = send_len;
        client_msg_record.info.msg_type = MSG_TYPE_NOTIF;
        client_msg_record.info.src_pid = current_cap_group->pid;
        client_msg_record.info.src_tid = current_thread->cap;
        if (current_cap_group->pid == GTASK_PID
            && current_thread->cap == GTASK_TID) {
                client_msg_record.info.src_pid = 0;
                client_msg_record.info.src_tid = 0;
        }

        ret = __tee_msg_send(channel, &client_msg_record);
        if (ret != 0) {
                goto out_free_ksend_buf;
        }

        return 0;

out_free_ksend_buf:
        kfree(ksend_buf);

out_fail:
        return ret;
}

/* Wake up all blocking clients in the msg_queue */
static int __wake_up_all_clients(struct channel *channel)
{
        struct msg_entry *entry;
        struct thread *client;

        for_each_in_list (
                entry, struct msg_entry, msg_queue_node, &channel->msg_queue) {
                if (entry->client_msg_record.info.msg_type == MSG_TYPE_CALL) {
                        client = entry->client_msg_record.client;
                        BUG_ON(!thread_is_ts_blocking(client)
                               && !thread_is_exited(client));
                        if (thread_is_ts_blocking(client)) {
                                arch_set_thread_return(client, -EINVAL);
                                BUG_ON(sched_enqueue(client));
                        }
                }
        }

        return 0;
}

/*
 * Destroy waiting nodes in the msg queue of the given channel
 * which belong to the given cap_group
 */
static int __destory_waiting_node(struct channel *channel,
                                  struct cap_group *cap_group)
{
        struct msg_entry *entry;

        for_each_in_list (
                entry, struct msg_entry, msg_queue_node, &channel->msg_queue) {
                if (entry->client_msg_record.client->cap_group == cap_group) {
                        list_del(&entry->msg_queue_node);
                        kfree(entry->client_msg_record.ksend_buf);
                        kfree(entry);
                }
        }

        return 0;
}

/*
 * close_channel will be called if
 * 1. channel's creater calls ot_sys_tee_msg_stop_channel
 * 2. recycler calls sys_cap_group_recycle
 */
int close_channel(struct channel *channel, struct cap_group *cap_group)
{
        lock(&channel->lock);
        if (channel->creater == cap_group) {
                channel->state = CHANNEL_INVALID;
                __wake_up_all_clients(channel);
        } else {
                __destory_waiting_node(channel, cap_group);
        }
        unlock(&channel->lock);
        return 0;
}

cap_t ot_sys_tee_msg_create_msg_hdl(void)
{
        struct msg_hdl *msg_hdl = NULL;
        int msg_hdl_cap = 0;
        int ret = 0;

        msg_hdl = obj_alloc(TYPE_MSG_HDL, sizeof(*msg_hdl));
        if (!msg_hdl) {
                ret = -ENOMEM;
                goto out_fail;
        }

        /* init msg_hdl */
        lock_init(&msg_hdl->lock);

        msg_hdl_cap = cap_alloc(current_cap_group, msg_hdl);
        if (msg_hdl_cap < 0) {
                ret = msg_hdl_cap;
                goto out_free_obj;
        }

        return msg_hdl_cap;

out_free_obj:
        obj_free(msg_hdl);

out_fail:
        return ret;
}

cap_t ot_sys_tee_msg_create_channel(void)
{
        struct channel *channel = NULL;
        int channel_cap = 0;
        int ret = 0;

        channel = obj_alloc(TYPE_CHANNEL, sizeof(*channel));
        if (!channel) {
                ret = -ENOMEM;
                goto out_fail;
        }

        /* init channel */
        lock_init(&channel->lock);
        init_list_head(&channel->msg_queue);
        init_list_head(&channel->thread_queue);
        channel->creater = current_cap_group;
        channel->state = CHANNEL_VALID;

        channel_cap = cap_alloc(current_cap_group, channel);
        if (channel_cap < 0) {
                ret = channel_cap;
                goto out_free_obj;
        }

        return channel_cap;

out_free_obj:
        obj_free(channel);

out_fail:
        return ret;
}

int ot_sys_tee_msg_stop_channel(cap_t channel_cap)
{
        struct channel *channel;
        int ret;

        channel = obj_get(current_cap_group, channel_cap, TYPE_CHANNEL);
        if (channel == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_channel;
        }

        if (channel->creater == current_cap_group) {
                ret = close_channel(channel, current_cap_group);
        } else {
                ret = -EINVAL;
        }

        obj_put(channel);

out_fail_get_channel:
        return ret;
}

int ot_sys_tee_msg_receive(cap_t channel_cap, void *recv_buf, size_t recv_len,
                           cap_t msg_hdl_cap, void *info, int timeout)
{
        struct channel *channel;
        struct msg_hdl *msg_hdl;
        int ret;

        if (check_user_addr_range((vaddr_t)recv_buf, recv_len) != 0) {
                return -EINVAL;
        }
        if (check_user_addr_range((vaddr_t)info, sizeof(struct src_msginfo))
            != 0) {
                return -EINVAL;
        }

        channel = obj_get(current_cap_group, channel_cap, TYPE_CHANNEL);
        if (channel == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_channel;
        }
        msg_hdl = obj_get(current_cap_group, msg_hdl_cap, TYPE_MSG_HDL);
        if (msg_hdl == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_msg_hdl;
        }

        /* Assume __tee_msg_receive will obj_put channel & msg_hdl if noreturn
         */
        ret = __tee_msg_receive(
                channel, recv_buf, recv_len, msg_hdl, info, timeout);

        obj_put(msg_hdl);

out_fail_get_msg_hdl:
        obj_put(channel);

out_fail_get_channel:
        return ret;
}

int ot_sys_tee_msg_call(cap_t channel_cap, void *send_buf, size_t send_len,
                        void *recv_buf, size_t recv_len,
                        struct timespec *timeout)
{
        struct channel *channel;
        int ret;

        if (check_user_addr_range((vaddr_t)send_buf, send_len) != 0) {
                return -EINVAL;
        }
        if (check_user_addr_range((vaddr_t)recv_buf, recv_len) != 0) {
                return -EINVAL;
        }

        channel = obj_get(current_cap_group, channel_cap, TYPE_CHANNEL);
        if (channel == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_channel;
        }

        /* Assume __tee_msg_call will obj_put channel if noreturn */
        ret = __tee_msg_call(
                channel, send_buf, send_len, recv_buf, recv_len, timeout);

        obj_put(channel);

out_fail_get_channel:
        return ret;
}

int ot_sys_tee_msg_reply(cap_t msg_hdl_cap, void *reply_buf, size_t reply_len)
{
        struct msg_hdl *msg_hdl;
        int ret;

        if (check_user_addr_range((vaddr_t)reply_buf, reply_len) != 0) {
                return -EINVAL;
        }

        msg_hdl = obj_get(current_cap_group, msg_hdl_cap, TYPE_MSG_HDL);
        if (msg_hdl == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_msg_hdl;
        }

        ret = __tee_msg_reply(msg_hdl, reply_buf, reply_len);

        obj_put(msg_hdl);

out_fail_get_msg_hdl:
        return ret;
}

int ot_sys_tee_msg_notify(cap_t channel_cap, void *send_buf, size_t send_len)
{
        struct channel *channel;
        int ret;

        if (check_user_addr_range((vaddr_t)send_buf, send_len) != 0) {
                return -EINVAL;
        }

        channel = obj_get(current_cap_group, channel_cap, TYPE_CHANNEL);
        if (channel == NULL) {
                ret = -ECAPBILITY;
                goto out_fail_get_channel;
        }

        ret = __tee_msg_notify(channel, send_buf, send_len);

        obj_put(channel);

out_fail_get_channel:
        return ret;
}

void channel_deinit(void *ptr)
{
        struct msg_entry *entry;
        struct channel *channel;

        channel = (struct channel *)ptr;

        for_each_in_list (
                entry, struct msg_entry, msg_queue_node, &channel->msg_queue) {
                list_del(&entry->msg_queue_node);
                kfree(entry->client_msg_record.ksend_buf);
                kfree(entry);
        }
}

void msg_hdl_deinit(void *ptr)
{
}
