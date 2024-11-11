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

#include <chcore/container/hashtable.h>
#include <string.h>
#include <assert.h>
#include <chcore/syscall.h>

#include <ipclib.h>

#include <chanmgr.h>

#define CHANMGR_DEFAULT_HTABLE_SIZE 1024
#define HASH_BASE                   998244353

#define info(fmt, ...) \
        printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)

/* check name empty at ipc entry */
static inline bool __chan_name_empty(char *name)
{
        /* to avoid overflow */
        name[CHAN_REQ_NAME_LEN - 1] = 0;
        return strlen(name) == 0;
}

static uint32_t __hash_name(const char *name)
{
        size_t i, len;
        uint32_t hashsum = 0;

        len = strlen(name);
        for (i = 0; i < len; i++) {
                hashsum = hashsum * HASH_BASE + (uint32_t)name[i];
        }

        return hashsum;
}

static inline uint32_t __hash_taskid(uint32_t taskid, int ch_num)
{
        return taskid * HASH_BASE + ch_num;
}

static inline bool __ch_num_valid(int ch_num)
{
        return ch_num == 0 || ch_num == 1;
}

struct channel_entry {
        struct hlist_node name2chan_node;
        struct hlist_node cid2chan_node;
        struct hlist_node badge2chan_node;
        badge_t badge;
        char *name;
        int cap;
        uint32_t taskid;
        int ch_num;
        struct reg_items_st reg_items;
};

static struct chanmgr chanmgr;

void channel_entry_init(struct channel_entry *entry)
{
        init_hlist_node(&entry->name2chan_node);
        init_hlist_node(&entry->cid2chan_node);
        init_hlist_node(&entry->badge2chan_node);
        entry->name = NULL;
        entry->ch_num = -1;
        entry->cap = -1;
        entry->taskid = 0;
        entry->badge = 0;
}

void channel_entry_deinit(struct channel_entry *entry)
{
        if (entry->name) {
                free(entry->name);
        }
}

int chanmgr_init(void)
{
        int ret;

        init_htable(&chanmgr.name2chan, CHANMGR_DEFAULT_HTABLE_SIZE);
        init_htable(&chanmgr.cid2chan, CHANMGR_DEFAULT_HTABLE_SIZE);
        init_htable(&chanmgr.badge2chan, CHANMGR_DEFAULT_HTABLE_SIZE);
        ret = pthread_mutex_init(&chanmgr.lock, NULL);

        return ret;
}

void chanmgr_deinit(void)
{
}

static struct channel_entry *__get_entry_by_name(const char *name)
{
        struct hlist_head *bucket;
        struct channel_entry *entry;
        uint32_t hashsum;

        hashsum = __hash_name(name);
        bucket = htable_get_bucket(&chanmgr.name2chan, hashsum);
        for_each_in_hlist (entry, name2chan_node, bucket) {
                if (entry->name != NULL && strcmp(name, entry->name) == 0) {
                        return entry;
                }
        }

        return NULL;
}

static struct channel_entry *__get_entry_by_cid(uint32_t taskid, int ch_num)
{
        struct hlist_head *bucket;
        struct channel_entry *entry;
        uint32_t hashsum;

        if (!__ch_num_valid(ch_num)) {
                return NULL;
        }

        hashsum = __hash_taskid(taskid, ch_num);
        bucket = htable_get_bucket(&chanmgr.cid2chan, hashsum);
        for_each_in_hlist (entry, cid2chan_node, bucket) {
                if (entry->taskid == taskid && entry->ch_num == ch_num) {
                        return entry;
                }
        }

        return NULL;
}

void chanmgr_handle_create_channel(ipc_msg_t *ipc_msg, badge_t badge, int pid,
                                   int tid)
{
        struct channel_entry *entry;
        int ret, cap;
        size_t len;
        uint32_t hashsum;
        struct reg_items_st reg_items;
        struct chan_request *req =
                (struct chan_request *)ipc_get_msg_data(ipc_msg);
        uint32_t taskid = pid_to_taskid(tid, pid);

        info("badge %x pid %x tid %x\n", badge, pid, tid);

        pthread_mutex_lock(&chanmgr.lock);

        memcpy(&reg_items,
               &req->create_channel.reg_items,
               sizeof(struct reg_items_st));
        if (__chan_name_empty(req->create_channel.name)) {
                reg_items.reg_name = false;
                reg_items.reg_tamgr = false;
        }
        if (!__ch_num_valid(req->create_channel.ch_num)) {
                reg_items.reg_pid = false;
        }
        if (!reg_items.reg_name && !reg_items.reg_pid && !reg_items.reg_tamgr) {
                ret = -EINVAL;
                goto out_fail;
        }

        if (reg_items.reg_name || reg_items.reg_tamgr) {
                entry = __get_entry_by_name(req->create_channel.name);
                if (entry != NULL) {
                        ret = -EEXIST;
                        goto out_fail;
                }
        }
        if (reg_items.reg_pid) {
                entry = __get_entry_by_cid(taskid, req->create_channel.ch_num);
                if (entry != NULL) {
                        ret = -EEXIST;
                        goto out_fail;
                }
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out_fail;
        }
        channel_entry_init(entry);

        cap = (int)ipc_get_msg_cap(ipc_msg, 0);
        if (cap < 0) {
                ret = -EINVAL;
                goto out_free_entry;
        }
        entry->cap = cap;
        entry->taskid = taskid;
        entry->badge = badge;

        if (reg_items.reg_name || reg_items.reg_tamgr) {
                len = strlen(req->create_channel.name);
                entry->name = malloc(len + 1);
                if (entry->name == NULL) {
                        ret = -ENOMEM;
                        goto out_free_entry;
                }
                memcpy(entry->name, req->create_channel.name, len);
                entry->name[len] = 0;
        }
        if (reg_items.reg_pid) {
                entry->ch_num = req->create_channel.ch_num;
        }

        memcpy(&entry->reg_items, &reg_items, sizeof(struct reg_items_st));

        htable_add(&chanmgr.badge2chan, badge, &entry->badge2chan_node);
        if (reg_items.reg_name || reg_items.reg_tamgr) {
                hashsum = __hash_name(entry->name);
                htable_add(&chanmgr.name2chan, hashsum, &entry->name2chan_node);
        }
        if (reg_items.reg_pid) {
                hashsum = __hash_taskid(entry->taskid, entry->ch_num);
                htable_add(&chanmgr.cid2chan, hashsum, &entry->cid2chan_node);
        }

        pthread_mutex_unlock(&chanmgr.lock);
        ret = 0;
        ipc_return(ipc_msg, ret);

out_free_entry:
        free(entry);

out_fail:
        pthread_mutex_unlock(&chanmgr.lock);
        ipc_return(ipc_msg, ret);
}

void chanmgr_handle_remove_channel(ipc_msg_t *ipc_msg, badge_t badge, int pid,
                                   int tid)
{
        int ret;
        struct channel_entry *entry = NULL;
        struct chan_request *req =
                (struct chan_request *)ipc_get_msg_data(ipc_msg);

        info("badge %x pid %x tid %x\n", badge, pid, tid);

        pthread_mutex_lock(&chanmgr.lock);

        if (!__chan_name_empty(req->remove_channel.name)) {
                entry = __get_entry_by_name(req->remove_channel.name);
        }
        if (entry == NULL && __ch_num_valid(req->remove_channel.ch_num)) {
                entry = __get_entry_by_cid(req->remove_channel.taskid,
                                           req->remove_channel.ch_num);
        }
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }
        if (badge != entry->badge) {
                ret = -EINVAL;
                goto out;
        }

        ret = usys_revoke_cap(entry->cap, false);

        hlist_del(&entry->badge2chan_node);
        if (entry->reg_items.reg_name || entry->reg_items.reg_tamgr) {
                hlist_del(&entry->name2chan_node);
        }
        if (entry->reg_items.reg_pid) {
                hlist_del(&entry->cid2chan_node);
        }
        channel_entry_deinit(entry);
        free(entry);

out:
        pthread_mutex_unlock(&chanmgr.lock);
        ipc_return(ipc_msg, ret);
}

void chanmgr_handle_hunt_by_name(ipc_msg_t *ipc_msg, int pid, int tid)
{
        int ret;
        struct channel_entry *entry = NULL;
        struct chan_request *req =
                (struct chan_request *)ipc_get_msg_data(ipc_msg);

        info("pid %x tid %x\n", pid, tid);

        pthread_mutex_lock(&chanmgr.lock);

        if (__chan_name_empty(req->hunt_by_name.name)) {
                ret = -EINVAL;
                goto out;
        }

        entry = __get_entry_by_name(req->hunt_by_name.name);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        if (entry->reg_items.reg_tamgr) {
                req->hunt_by_name.taskid = entry->taskid;
                /* TODO: gtask's taskid is 0 */
                if (req->hunt_by_name.taskid == GTASK_TASKID) {
                        req->hunt_by_name.taskid = 0;
                }
                ret = 0;
        } else {
                ret = -ENOENT;
        }
out:
        pthread_mutex_unlock(&chanmgr.lock);
        ipc_return(ipc_msg, ret);
}

void chanmgr_handle_get_ch_from_path(ipc_msg_t *ipc_msg, int pid, int tid)
{
        int ret;
        struct channel_entry *entry = NULL;
        struct chan_request *req =
                (struct chan_request *)ipc_get_msg_data(ipc_msg);

        info("pid %x tid %x\n", pid, tid);

        pthread_mutex_lock(&chanmgr.lock);

        if (__chan_name_empty(req->get_ch_from_path.name)) {
                ret = -EINVAL;
                goto out;
        }

        entry = __get_entry_by_name(req->get_ch_from_path.name);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ret = ipc_set_msg_cap(ipc_msg, 0, entry->cap);
        if (ret < 0) {
                ret = -EINVAL;
        } else {
                ret = 0;
        }

out:
        pthread_mutex_unlock(&chanmgr.lock);
        if (ret == 0) {
                ipc_return_with_cap(ipc_msg, ret);
        } else {
                ipc_return(ipc_msg, ret);
        }
}

void chanmgr_handle_get_ch_from_taskid(ipc_msg_t *ipc_msg, int pid, int tid)
{
        int ret;
        struct channel_entry *entry = NULL;
        struct chan_request *req =
                (struct chan_request *)ipc_get_msg_data(ipc_msg);

        info("pid %x tid %x\n", pid, tid);

        pthread_mutex_lock(&chanmgr.lock);

        if (!__ch_num_valid(req->get_ch_from_taskid.ch_num)) {
                ret = -EINVAL;
                goto out;
        }
        /* TODO: gtask's taskid is 0 */
        if (req->get_ch_from_taskid.taskid == 0) {
                req->get_ch_from_taskid.taskid = GTASK_TASKID;
        }
        entry = __get_entry_by_cid(req->get_ch_from_taskid.taskid,
                                   req->get_ch_from_taskid.ch_num);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ret = ipc_set_msg_cap(ipc_msg, 0, entry->cap);
        if (ret < 0) {
                ret = -EINVAL;
        } else {
                ret = 0;
        }

out:
        pthread_mutex_unlock(&chanmgr.lock);
        if (ret == 0) {
                ipc_return_with_cap(ipc_msg, ret);
        } else {
                ipc_return(ipc_msg, ret);
        }
}

void chanmgr_destructor(badge_t client_badge)
{
        struct hlist_head *bucket;
        struct channel_entry *entry, *tmp;

        info("badge %x\n", client_badge);

        pthread_mutex_lock(&chanmgr.lock);

        bucket = htable_get_bucket(&chanmgr.badge2chan, client_badge);
        for_each_in_hlist_safe (entry, tmp, badge2chan_node, bucket) {
                if (entry->badge == client_badge) {
                        usys_revoke_cap(entry->cap, false);

                        hlist_del(&entry->badge2chan_node);
                        if (entry->reg_items.reg_name
                            || entry->reg_items.reg_tamgr) {
                                hlist_del(&entry->name2chan_node);
                        }
                        if (entry->reg_items.reg_pid) {
                                hlist_del(&entry->cid2chan_node);
                        }
                        channel_entry_deinit(entry);
                        free(entry);
                }
        }

        pthread_mutex_unlock(&chanmgr.lock);
}
