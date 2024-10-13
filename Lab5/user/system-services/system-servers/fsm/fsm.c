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

#include "fsm.h"
#include "chcore/ipc.h"
#include <errno.h>
#include <chcore/container/list.h>
#include "device.h"
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <chcore-internal/procmgr_defs.h>
#include <chcore/string.h>
#include <chcore/pthread.h>

#include "fsm_client_cap.h"
#include "mount_info.h"
#include "mount_disk_fs.h"

int fs_num = 0;
pthread_mutex_t fsm_client_cap_table_lock;
pthread_rwlock_t mount_point_infos_rwlock;

/* Initialize when fsm start */
static inline void init_utils(void)
{
        init_list_head(&fsm_client_cap_table);
        pthread_mutex_init(&fsm_client_cap_table_lock, NULL);
        init_list_head(&mount_point_infos);
        pthread_rwlock_init(&mount_point_infos_rwlock, NULL);
}

static void fsm_destructor(badge_t client_badge)
{
        struct fsm_client_cap_node *n, *iter_tmp;
        pthread_mutex_lock(&fsm_client_cap_table_lock);

        for_each_in_list_safe (n, iter_tmp, node, &fsm_client_cap_table) {
                if (n->client_badge == client_badge) {
                        list_del(&n->node);
                        free(n);
                        break;
                }
        }

        pthread_mutex_unlock(&fsm_client_cap_table_lock);
}

int init_fsm(void)
{
        int ret;

        /* Initialize */
        init_utils();

        ret = fsm_mount_fs("/tmpfs.srv", "/");
        if (ret < 0) {
                error("failed to mount tmpfs, ret %d\n", ret);
                usys_exit(-1);
        }

        return 0;
}

static int boot_tmpfs(void)
{
        struct proc_request pr;
        ipc_msg_t *ipc_msg;
        int ret;

        ipc_msg =
                ipc_create_msg(procmgr_ipc_struct, sizeof(struct proc_request));
        pr.req = PROC_REQ_GET_SERVICE_CAP;
        strncpy(pr.get_service_cap.service_name, SERVER_TMPFS,
                SERVICE_NAME_LEN);

        ipc_set_msg_data(ipc_msg, &pr, 0, sizeof(pr));
        ret = ipc_call(procmgr_ipc_struct, ipc_msg);

        if (ret < 0) {
                goto out;
        }

        ret = ipc_get_msg_cap(ipc_msg, 0);
out:
        ipc_destroy_msg(ipc_msg);
        return ret;
}

int fsm_mount_fs(const char *path, const char *mount_point)
{
        cap_t fs_cap;
        int ret;
        struct mount_point_info_node *mp_node;

        ret = -1;
        if (fs_num == MAX_FS_NUM) {
                error("maximal number of FSs is reached: %d\n", fs_num);
                goto out;
        }

        if (strlen(mount_point) > MAX_MOUNT_POINT_LEN) {
                error("mount point too long: > %d\n", MAX_MOUNT_POINT_LEN);
                goto out;
        }

        if (mount_point[0] != '/') {
                error("mount point should start with '/'\n");
                goto out;
        }

        if (strcmp(path, "/tmpfs.srv") == 0) {
                /* @fs_cap is 0 -> means launch TMPFS */
                info("Mounting fs from local binary: %s...\n", path);

                fs_cap = boot_tmpfs();

                if (fs_cap <= 0) {
                        info("Fails to launch TMPFS, which returns %d\n", ret);
                        goto out;
                }

                pthread_rwlock_wrlock(&mount_point_infos_rwlock);
                mp_node = set_mount_point("/", 1, fs_cap);
                info("TMPFS is up, with cap = %d\n", fs_cap);
        } else {
                fs_cap = mount_storage_device(path);
                if (fs_cap < 0) {
                        ret = -errno;
                        goto out;
                }
                pthread_rwlock_wrlock(&mount_point_infos_rwlock);
                mp_node = set_mount_point(
                        mount_point, strlen(mount_point), fs_cap);
        }

        /* Lab 5 TODO Begin (Part 1) */
        UNUSED(mp_node);
        pthread_rwlock_unlock(&mount_point_infos_rwlock);
        /* Lab 5 TODO End (Part 1) */

out:
        return ret;
}

/*
 * @args: 'path' is device name, like 'sda1'...
 * send FS_REQ_UMOUNT to corresponding fs_server
 */
int fsm_umount_fs(const char *path)
{
        cap_t fs_cap;
        int ret;
        ipc_msg_t *ipc_msg;
        ipc_struct_t *ipc_struct;
        struct fs_request *fr_ptr;

        pthread_rwlock_wrlock(&mount_point_infos_rwlock);

        /* get corresponding fs_server_cap by device name */
        fs_cap = mount_storage_device(path);

        ipc_struct = ipc_register_client(fs_cap);
        ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct fs_request));
        fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

        fr_ptr->req = FS_REQ_UMOUNT;

        ipc_set_msg_data(ipc_msg, (char *)fr_ptr, 0, sizeof(struct fs_request));
        ret = ipc_call(ipc_struct, ipc_msg);
        ipc_destroy_msg(ipc_msg);

        pthread_rwlock_unlock(&mount_point_infos_rwlock);

        return ret;
}

/*
 * @args: 'path' is device name, like 'sda1'...
 * send FS_REQ_UMOUNT to corresponding fs_server
 */
int fsm_sync_page_cache(void)
{
        ipc_msg_t *ipc_msg;
        ipc_struct_t *ipc_struct;
        struct fs_request *fr_ptr;
        struct mount_point_info_node *iter;
        int ret = 0;

        for_each_in_list (
                iter, struct mount_point_info_node, node, &mount_point_infos) {
                ipc_struct = iter->_fs_ipc_struct;
                ipc_msg = ipc_create_msg(ipc_struct, sizeof(struct fs_request));
                fr_ptr = (struct fs_request *)ipc_get_msg_data(ipc_msg);

                fr_ptr->req = FS_REQ_SYNC;

                ret = ipc_call(ipc_struct, ipc_msg);
                ipc_destroy_msg(ipc_msg);
                if (ret != 0) {
                        printf("Failed to sync in %s\n", iter->path);
                        goto out;
                }
        }

out:
        return ret;
}

/*
 * Types in the following two functions would conflict with existing builds,
 * I suggest to move the tmpfs code out of kernel tree to resolve this.
 */

DEFINE_SERVER_HANDLER(fsm_dispatch)
{
        int ret = 0;
        struct fsm_request *fsm_req;
        struct mount_point_info_node *mpinfo;
        int mount_id;
        bool ret_with_cap = false;

        if (ipc_msg == NULL) {
                ipc_return(ipc_msg, -EINVAL);
        }

        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);
        switch (fsm_req->req) {
        case FSM_REQ_PARSE_PATH: {
                // Get Corresponding MOUNT_INFO
                /* Lab 5 TODO Begin (Part 1) */
                UNUSED(mpinfo);
                UNUSED(mount_id);
                /* Lab 5 TODO End (Part 1) */
                break;
        }
        case FSM_REQ_MOUNT: {
                // path=(device_name), path2=(mount_point)
                ret = fsm_mount_fs(fsm_req->path, fsm_req->mount_path);
                break;
        }
        case FSM_REQ_UMOUNT: {
                ret = fsm_umount_fs(fsm_req->path);
                break;
        }
        case FSM_REQ_SYNC: {
                ret = fsm_sync_page_cache();
                break;
        }
        default:
                error("%s: %d Not impelemented yet\n",
                        __func__,
                        ((int *)(ipc_get_msg_data(ipc_msg)))[0]);
                usys_exit(-1);
                break;
        }

        if (ret_with_cap)
                ipc_return_with_cap(ipc_msg, ret);
        else
                ipc_return(ipc_msg, ret);
}

static void* fsm_server_thread_for_itself(void* args){
        info("[FSM] register server value = %u\n",
             ipc_register_server_with_destructor(fsm_dispatch,
                                                 DEFAULT_CLIENT_REGISTER_HANDLER,
                                                 fsm_destructor));
        return NULL;
}

int main(int argc, char *argv[], char *envp[])
{
        cap_t server_cap;
        init_fsm();
        pthread_t fsm_server_thread_id;
        info("[FSM] register server value = %u\n",
             ipc_register_server_with_destructor(fsm_dispatch,
                                                 DEFAULT_CLIENT_REGISTER_HANDLER,
                                                 fsm_destructor));
        server_cap = chcore_pthread_create(&fsm_server_thread_id, NULL, fsm_server_thread_for_itself, NULL);
        mount_disk_fs(server_cap);
        usys_exit(0);
        return 0;
}
