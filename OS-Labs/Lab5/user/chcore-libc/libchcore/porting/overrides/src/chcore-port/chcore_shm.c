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

#include <sys/shm.h>
#include <stdint.h>
#include <stdio.h>
#include <bits/errno.h>
#include <chcore/syscall.h>
#include <chcore/ipc.h>
#include <chcore/memory.h>
#include <chcore-internal/shmmgr_defs.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/services.h>
#include <stdlib.h>
#include <pthread.h>
#include <debug_lock.h>
#include <errno.h>

#include "chcore_shm.h"

#ifdef CHCORE_SERVER_POSIX_SHM /* Enable the support for shmxxx */

ipc_struct_t *shmmgr_ipc_struct = NULL;
static int volatile connect_shmmgr_lock = 0;

static struct list_head shmat_info;
static int volatile shmat_info_init = 0;
static pthread_mutex_t shmat_info_lock;

/*
 * Get shmid by key.
 * Return shm_cap as shmid.
 * Only support IPC_CREAT now.
 */
int chcore_shmget(key_t key, size_t size, int flag)
{
        ipc_msg_t *shm_msg;
        struct shm_request *shm_req;
        int shm_id;
        int ret;

        /* If already connected to shmmgr, skip connection */
        chcore_spin_lock(&connect_shmmgr_lock);
        if (!shmmgr_ipc_struct) {
                // printf("connect_to_shmmgr\n");
                shmmgr_ipc_struct = chcore_conn_srv(SERVER_SYSTEMV_SHMMGR);
        }
        chcore_spin_unlock(&connect_shmmgr_lock);

        /* Send IPC to shmmgr to get shm_cap */
        shm_msg = ipc_create_msg(
                shmmgr_ipc_struct, sizeof(struct shm_request));
        shm_req = (struct shm_request *)ipc_get_msg_data(shm_msg);
        shm_req->req = SHM_REQ_GET;
        shm_req->shm_get.flag = flag;
        shm_req->shm_get.key = key;
        shm_req->shm_get.size = size;

        ret = ipc_call(shmmgr_ipc_struct, shm_msg);

        if (ret < 0) {
                /* if cap < 0, ret should also be < 0 */
                errno = -ret;
                ipc_destroy_msg(shm_msg);
                return -1;
        }

        /* Result info is stored in shm_req */
        shm_id = shm_req->result.shmid;
        ipc_destroy_msg(shm_msg);
        return shm_id;
}

static struct local_shm_record *create_local_record(cap_t shm_cap, int id,
                                                    unsigned long addr)
{
        struct local_shm_record *record;
        record = (struct local_shm_record *)malloc(sizeof(*record));
        if (!record)
                return NULL;

        record->shm_cap = shm_cap;
        record->shmid = id;
        record->addr = addr;
        return record;
}

static void delete_local_record(struct local_shm_record *record)
{
        usys_revoke_cap(record->shm_cap, false);
        list_del(&(record->node));
        free(record);
}

static void ensure_shmat_info_list_init(void)
{
        if (!shmat_info_init) {
                pthread_mutex_lock(&shmat_info_lock);
                if (!shmat_info_init) {
                        init_list_head(&shmat_info);
                        __sync_synchronize();
                        shmat_info_init = 1;
                }
                pthread_mutex_unlock(&shmat_info_lock);
        }
}

void *chcore_shmat(int id, const void *addr, int flag)
{
        if (flag & SHM_RND) {
                printf("[Shm] No support for SHM_RND now.\n");
                goto out;
        }
        if (!shmmgr_ipc_struct) {
                errno = ENOENT;
                goto out;
        }
        ipc_msg_t *shm_msg;
        struct shm_request *shm_req;
        cap_t shm_cap;
        int ret = 0;
        struct local_shm_record *record = NULL;
        void *shmaddr;
        unsigned long perm = 0;

        ensure_shmat_info_list_init();

        shm_msg = ipc_create_msg(
                shmmgr_ipc_struct, sizeof(struct shm_request));
        shm_req = (struct shm_request *)ipc_get_msg_data(shm_msg);
        shm_req->req = SHM_REQ_AT;
        shm_req->shm_at.shmid = id;
        ret = ipc_call(shmmgr_ipc_struct, shm_msg);
        if (ret < 0) {
                errno = -ret;
                ipc_destroy_msg(shm_msg);
                goto out;
        }
        shm_cap = ipc_get_msg_cap(shm_msg, 0);
        ipc_destroy_msg(shm_msg);

        record = create_local_record(shm_cap, id, (unsigned long)NULL);
        if (!record) {
                errno = ENOMEM;
                goto out;
        }

        /* Set the permission */
        if (flag & SHM_RDONLY) {
                perm = VMR_READ;
        } else if (flag & SHM_EXEC) {
                perm = VMR_READ | VMR_WRITE | VMR_EXEC;
        } else {
                perm = VMR_READ | VMR_WRITE;
        }

        if (!addr) {
                shmaddr = chcore_auto_map_pmo(
                        shm_cap, shm_req->result.size, perm);
                if (shmaddr == NULL) {
                        /* errno is set in chcore_auto_map_pmo */
                        ret = -1;
                }
        } else {
                ret = usys_map_pmo(
                        SELF_CAP, shm_cap, (unsigned long)addr, perm);
                shmaddr = (void *)addr;
        }
        if (ret < 0) {
                errno = ret;
                delete_local_record(record);
                goto out;
        } else
                record->addr = (unsigned long)shmaddr;

        pthread_mutex_lock(&shmat_info_lock);
        list_add(&(record->node), &shmat_info);

        pthread_mutex_unlock(&shmat_info_lock);
        return shmaddr;

out:
        return (void *)-1;
}

int chcore_shmdt(const void *addr)
{
        if (!shmat_info_init) {
                pthread_mutex_lock(&shmat_info_lock);
                if (!shmat_info_init) {
                        errno = ENOENT;
                        pthread_mutex_unlock(&shmat_info_lock);
                        return -1;
                }
                pthread_mutex_unlock(&shmat_info_lock);
        }

        struct local_shm_record *record;
        cap_t cap;
        int exist = 0;
        int ret;

        pthread_mutex_lock(&shmat_info_lock);
        for_each_in_list (record, struct local_shm_record, node, &shmat_info) {
                if (record->addr == (unsigned long)addr) {
                        exist = 1;
                        cap = record->shm_cap;
                        break;
                }
        }

        /* Not exist */
        if (exist == 0) {
                errno = EINVAL;
                ret = -1;
                goto out;
        }

        ret = usys_unmap_pmo(SELF_CAP, cap, (unsigned long)addr);
        if (ret < 0) {
                printf("failed shmdt cap:%d\n", cap);
                errno = -ret;
                goto out;
        }
        delete_local_record(record);

out:
        pthread_mutex_unlock(&shmat_info_lock);
        return ret;
}

int chcore_shmctl(int id, int cmd, struct shmid_ds *buf)
{
        if (!shmmgr_ipc_struct) {
                errno = ENOENT;
                return -1;
        }
        int ret = 0;
        ipc_msg_t *shm_msg;
        struct shm_request *shm_req;

        shm_msg = ipc_create_msg(
                shmmgr_ipc_struct, sizeof(struct shm_request));
        shm_req = (struct shm_request *)ipc_get_msg_data(shm_msg);
        shm_req->req = SHM_REQ_CTL;
        shm_req->shm_ctl.shmid = id;
        shm_req->shm_ctl.cmd = cmd;
        ret = ipc_call(shmmgr_ipc_struct, shm_msg);
        if (ret < 0) {
                errno = -ret;
                ret = -1;
        }
        ipc_destroy_msg(shm_msg);

        return ret;
}

#endif
