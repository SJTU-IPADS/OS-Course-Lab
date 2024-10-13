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

#include <oh_mem_ops.h>
#include <chcore/syscall.h>
#include <proc_node.h>
#include <chcore/container/hashtable.h>
#include <string.h>
#include <pthread.h>

#include <tee_uuid.h>
#include <ipclib.h>

#define DEFAULT_HTABLE_SIZE 1024

struct shm_entry {
        vaddr_t vaddr;
        pid_t pid;
        cap_t pmo;
        badge_t badge;
        struct tee_uuid uuid;
        struct hlist_node src2ent_node;
        struct hlist_node badge2ent_node;
};

static struct htable src2ent;
static pthread_mutex_t oh_shmmgr_lock;
static struct htable badge2ent;

void oh_mem_ops_init(void)
{
        pthread_mutex_init(&oh_shmmgr_lock, NULL);
        init_htable(&src2ent, DEFAULT_HTABLE_SIZE);
        init_htable(&badge2ent, DEFAULT_HTABLE_SIZE);
}

static struct shm_entry *__get_entry_by_src(vaddr_t vaddr, pid_t pid)
{
        struct hlist_head *bucket = htable_get_bucket(&src2ent, pid);
        struct shm_entry *entry;
        for_each_in_hlist (entry, src2ent_node, bucket) {
                if (entry->vaddr == vaddr && entry->pid == pid) {
                        return entry;
                }
        }
        return NULL;
}

void handle_transfer_pmo_cap(ipc_msg_t *ipc_msg, badge_t badge)
{
        struct proc_node *proc_node;
        struct proc_request *req;
        cap_t pmo, task_pmo;
        pid_t pid;
        int ret;

        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        pid = taskid_to_pid(req->transfer_pmo_cap.task_id);
        pmo = ipc_get_msg_cap(ipc_msg, 0);
        if (pmo < 0) {
                ret = pmo;
                goto out;
        }

        proc_node = get_proc_node_by_pid(pid);
        if (proc_node == NULL) {
                ret = -EINVAL;
                goto out_revoke_pmo;
        }

        ret = usys_transfer_caps(proc_node->proc_cap, &pmo, 1, &task_pmo);
        if (ret < 0) {
                goto out_revoke_pmo;
        }
        req->transfer_pmo_cap.pmo = task_pmo;

        put_proc_node(proc_node);
out_revoke_pmo:
        usys_revoke_cap(pmo, false);
out:
        ipc_return(ipc_msg, ret);
}

void handle_tee_alloc_sharemem(ipc_msg_t *ipc_msg, badge_t badge)
{
        int ret;
        cap_t target_pmo, self_pmo;
        struct proc_node *proc_node;
        struct proc_request *req;
        struct shm_entry *entry;

        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        proc_node = get_proc_node(badge);
        BUG_ON(proc_node == NULL);

        pthread_mutex_lock(&oh_shmmgr_lock);

        entry = __get_entry_by_src(req->tee_alloc_shm.vaddr, proc_node->pid);
        if (entry != NULL) {
                ret = -EEXIST;
                goto out;
        }

        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
                ret = -ENOMEM;
                goto out;
        }

        target_pmo = usys_create_tee_shared_pmo(
                proc_node->proc_cap, &req->tee_alloc_shm.uuid, req->tee_alloc_shm.size, &self_pmo);
        if (target_pmo < 0) {
                ret = target_pmo;
                goto out_free_entry;
        }

        entry->pid = proc_node->pid;
        entry->pmo = self_pmo;
        memcpy(&entry->uuid, &req->tee_alloc_shm.uuid, sizeof(struct tee_uuid));
        entry->vaddr = req->tee_alloc_shm.vaddr;
        entry->badge = badge;
        init_hlist_node(&entry->src2ent_node);
        init_hlist_node(&entry->badge2ent_node);
        htable_add(&src2ent, proc_node->pid, &entry->src2ent_node);
        htable_add(&badge2ent, badge, &entry->badge2ent_node);

        pthread_mutex_unlock(&oh_shmmgr_lock);
        put_proc_node(proc_node);
        ipc_return(ipc_msg, target_pmo);

out_free_entry:
        free(entry);
out:
        pthread_mutex_unlock(&oh_shmmgr_lock);
        put_proc_node(proc_node);
        ipc_return(ipc_msg, ret);
}

void handle_tee_get_sharemem(ipc_msg_t *ipc_msg, badge_t badge)
{
        int ret;
        struct proc_node *proc_node;
        struct proc_request *req;
        struct shm_entry *entry;

        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        proc_node = get_proc_node(badge);
        BUG_ON(proc_node == NULL);

        pthread_mutex_lock(&oh_shmmgr_lock);

        entry = __get_entry_by_src(req->tee_get_shm.vaddr, req->tee_get_shm.pid);
        if (entry == NULL) {
                ret = -ENOENT;
                goto out;
        }

        if (memcmp(&proc_node->puuid.uuid,
                   &entry->uuid,
                   sizeof(struct tee_uuid)) != 0) {
                ret = -EINVAL;
                goto out;
        }

        ipc_set_msg_return_cap_num(ipc_msg, 1);
        ipc_set_msg_cap(ipc_msg, 0, entry->pmo);

        pthread_mutex_unlock(&oh_shmmgr_lock);
        put_proc_node(proc_node);
        ipc_return_with_cap(ipc_msg, 0);

out:
        pthread_mutex_unlock(&oh_shmmgr_lock);
        put_proc_node(proc_node);
        ipc_return(ipc_msg, ret);
}

static void __destroy_entry(struct shm_entry *entry)
{
        htable_del(&entry->src2ent_node);
        htable_del(&entry->badge2ent_node);
        usys_revoke_cap(entry->pmo, false);
        free(entry);
}

void handle_tee_free_sharemem(ipc_msg_t *ipc_msg, badge_t badge)
{
        int ret = 0;
        struct proc_node *proc_node;
        struct proc_request *req;
        struct shm_entry *entry;

        req = (struct proc_request *)ipc_get_msg_data(ipc_msg);
        proc_node = get_proc_node(badge);

        pthread_mutex_lock(&oh_shmmgr_lock);

        entry = __get_entry_by_src(req->tee_free_shm.vaddr, proc_node->pid);
        if (entry == NULL) {
                goto out;
        }
        __destroy_entry(entry);

out:
        pthread_mutex_unlock(&oh_shmmgr_lock);
        put_proc_node(proc_node);
        ipc_return(ipc_msg, ret);
}

void clean_sharemem(badge_t badge)
{
        struct hlist_head *bucket = htable_get_bucket(&badge2ent, badge);
        struct shm_entry *entry, *tmp;
        
        pthread_mutex_lock(&oh_shmmgr_lock);
        for_each_in_hlist_safe (entry, tmp, badge2ent_node, bucket) {
                if (entry->badge == badge) {
                        __destroy_entry(entry);
                }
        }
        pthread_mutex_unlock(&oh_shmmgr_lock);
}
