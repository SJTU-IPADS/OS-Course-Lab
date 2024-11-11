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

/*
 * For POSIX-compatability, we support interfaces like shmxxx.
 * We implement them in user-level system servers, e.g., this posix_shm.
 */

#include <sys/ipc.h>
#include <bits/errno.h>
#include <chcore/type.h>
#include <chcore/container/list.h>
#include <chcore/ipc.h>
#include <chcore/bug.h>
#include <chcore/syscall.h>
#include <chcore-internal/shmmgr_defs.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>

struct shm_record {
        int key;
        size_t size;
        /* shmid is also the cap num in server */
        int shm_id;
        struct list_head node;
};

struct list_head global_shm_info;
/* Protect the global_shm_info list */
pthread_mutex_t shm_info_lock;

void init_global_shm_namespace(void)
{
        init_list_head(&global_shm_info);
        pthread_mutex_init(&shm_info_lock, NULL);
}

struct shm_record *create_record(int key, size_t size, int shm_cap)
{
        struct shm_record *record;
        record = (struct shm_record *)malloc(sizeof(*record));
        if (!record)
                return NULL;
        record->key = key;
        record->size = size;
        /* Use the cap num in server as its shmid */
        record->shm_id = shm_cap;
        return record;
}

void delete_record(struct shm_record *record)
{
        list_del(&(record->node));
        usys_revoke_cap(record->shm_id, false);
        free(record);
}

int handle_shmget(int key, size_t size, int flag, struct shm_request *msg)
{
        cap_t shm_cap;
        struct shm_record *record;
        int exist;
        int ret = 0;

        exist = 0;

        pthread_mutex_lock(&shm_info_lock);

        for_each_in_list (record, struct shm_record, node, &global_shm_info) {
                if (record->key == key) {
                        exist = 1;
                        break;
                }
        }

        if ((exist == 0) && !(flag & IPC_CREAT)) {
                ret = -ENOENT;
                goto out;
        }

        if ((exist == 1) && (flag & IPC_EXCL)) {
                ret = -EEXIST;
                goto out;
        }

        if (exist == 1) {
                ret = 0;
                msg->result.shmid = record->shm_id;
                goto out;
        }

        size = ROUND_UP(size, PAGE_SIZE);

        /* Allocate a PMO_SHM for the new shm_cap */
        shm_cap = usys_create_pmo(size, PMO_SHM);
        if (shm_cap < 0) {
                ret = -ENOMEM;
                goto out;
        }

        record = create_record(key, size, shm_cap);
        if (!record) {
                ret = -ENOMEM;
                usys_revoke_cap(shm_cap, false);
                goto out;
        }

        /* Record the shm_cap */
        list_add(&(record->node), &global_shm_info);

        msg->result.shmid = record->shm_id;

out:
        pthread_mutex_unlock(&shm_info_lock);
        return ret;
}

int get_shmat_info(int shmid, struct shm_request *msg)
{
        int ret = 0;
        struct shm_record *record;
        int exist = 0;

        pthread_mutex_lock(&shm_info_lock);

        for_each_in_list (record, struct shm_record, node, &global_shm_info) {
                if (record->shm_id == shmid) {
                        exist = 1;
                        msg->result.size = record->size;
                        msg->result.shm_cap = record->shm_id;
                        break;
                }
        }
        pthread_mutex_unlock(&shm_info_lock);

        if (exist == 0) {
                ret = -EINVAL;
        }

        return ret;
}

int handle_shmctl(int shmid, int cmd, struct shm_request *msg)
{
        struct shm_record *record;
        int exist = 0;

        if (cmd == IPC_RMID) {
                pthread_mutex_lock(&shm_info_lock);

                for_each_in_list (
                        record, struct shm_record, node, &global_shm_info) {
                        if (record->shm_id == shmid) {
                                exist = 1;
                                break;
                        }
                }

                if (exist)
                        delete_record(record);

                pthread_mutex_unlock(&shm_info_lock);

                if (exist == 0) {
                        return -EINVAL;
                }

                return 0;
        }

        return -EINVAL;
}

DEFINE_SERVER_HANDLER(shmmgr_dispatch)
{
        int key, size, flag, shmid, shmcmd;
        struct shm_request *record;
        int ret = 0;

        record = (struct shm_request *)ipc_get_msg_data(ipc_msg);

        switch (record->req) {
        case SHM_REQ_GET:
                key = record->shm_get.key;
                size = record->shm_get.size;
                flag = record->shm_get.flag;
                ret = handle_shmget(key, size, flag, record);
                ipc_return(ipc_msg, ret);
                break;
        case SHM_REQ_AT:
                shmid = record->shm_at.shmid;
                ret = get_shmat_info(shmid, record);
                ipc_set_msg_return_cap_num(ipc_msg, 1);
                ipc_set_msg_cap(ipc_msg, 0, record->result.shm_cap);
                ipc_return_with_cap(ipc_msg, ret);
                break;
        case SHM_REQ_CTL: // we only support RMID now.
                shmid = record->shm_ctl.shmid;
                shmcmd = record->shm_ctl.cmd;
                ret = handle_shmctl(shmid, shmcmd, record);
                ipc_return(ipc_msg, ret);
                break;
        default:
                printf("[Shm Manager] unvalid req: %x\n", record->req);
                break;
        }
        ipc_return(ipc_msg, ret);
}

int main(int argc, char *argv[], char *envp[])
{
        int ret;

        init_global_shm_namespace();

        ret = ipc_register_server(shmmgr_dispatch, register_cb_single);
        printf("[Shm Manager] register server value = %d\n", ret);

        usys_exit(0);
        return 0;
}
