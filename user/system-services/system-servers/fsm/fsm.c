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
#include <fcntl.h>

#include "fsm_client_cap.h"
#include "mount_info.h"
#include "mount_disk_fs.h"
#include "file_namespace.h"

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
        init_list_head(&fsm_client_namespace_table);
        pthread_rwlock_init(&fsm_client_namespace_table_rwlock, NULL);
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

        fsm_delete_file_namespace(client_badge);
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

        /* Connect to the FS that we mount now. */
        mp_node->_fs_ipc_struct = ipc_register_client(mp_node->fs_cap);

        if (mp_node->_fs_ipc_struct == NULL) {
                info("ipc_register_client failed\n");
                BUG_ON(remove_mount_point(mp_node->path) != 0);
                pthread_rwlock_unlock(&mount_point_infos_rwlock);
                goto out;
        }

        strlcpy(mp_node->path, mount_point, sizeof(mp_node->path));

        fs_num++;
        ret = 0;
        pthread_rwlock_unlock(&mount_point_infos_rwlock);

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

static int ns_get_full_path(const char *path, badge_t client_badge, int flags,
                            char *full_path, size_t full_path_len)
{
        struct file_namespace *file_ns;
        int ret;

        pthread_rwlock_rdlock(&fsm_client_namespace_table_rwlock);
        file_ns = fsm_get_file_namespace(client_badge);
        if (file_ns) {
                ret = parse_path_from_file_namespace(
                        file_ns, path, flags, full_path, full_path_len);
        } else {
                strncpy(full_path, path, full_path_len);
                full_path[full_path_len] = '\0';
                ret = 0;
        }

        pthread_rwlock_unlock(&fsm_client_namespace_table_rwlock);
        return ret;
}

static int get_mount_id(badge_t client_badge,
                        struct mount_point_info_node *mpinfo,
                        bool *ret_with_cap)
{
        int mount_id;
        pthread_mutex_lock(&fsm_client_cap_table_lock);
        mount_id = fsm_get_client_cap(client_badge, mpinfo->fs_cap);

        if (mount_id == -1) {
                /* Client not hold corresponding fs_cap */

                // Newly generated mount_id
                mount_id = fsm_set_client_cap(client_badge, mpinfo->fs_cap);
                *ret_with_cap = true;
        }

        pthread_mutex_unlock(&fsm_client_cap_table_lock);

        if (mount_id < 0) {
                *ret_with_cap = false;
        }

        return mount_id;
}

static int get_server_path(const char *full_path, badge_t client_badge,
                           char *server_path, size_t server_path_len,
                           struct mount_point_info_node **out_mpinfo)
{
        int mount_path_len;
        struct mount_point_info_node *mpinfo;

        pthread_rwlock_rdlock(&mount_point_infos_rwlock);
        mpinfo = get_mount_point(full_path, strlen(full_path));
        /* should always match the "/" mount point in current implementation */
        BUG_ON(mpinfo == NULL);

        mount_path_len = mpinfo->path_len;
        if (mount_path_len == 1)
                mount_path_len = 0;

        strncpy(server_path, full_path + mount_path_len, server_path_len);
        server_path[server_path_len] = '\0';
        /*
         * NOTE: Let mpinfo go out of mount_point_infos_rwlock, because in the
         * current implementation, once mpinfo is added to the global list, it
         * will not be removed anymore, which should not cause use-after-free.
         */
        *out_mpinfo = mpinfo;

        pthread_rwlock_unlock(&mount_point_infos_rwlock);
        return 0;
}

static int internal_parse_path(const char *path, badge_t client_badge,
                               int flags, char *server_path,
                               size_t server_path_len,
                               struct mount_point_info_node **mpinfo) {
        int ret;
        char full_path[FS_REQ_PATH_BUF_LEN];

        ret = ns_get_full_path(path, client_badge, flags, full_path, sizeof(full_path) - 1);
        if (ret < 0)
                goto out_fail;

        ret = get_server_path(full_path, client_badge,server_path, server_path_len, mpinfo);

out_fail:
        return ret;
}

int fsm_path_ops(ipc_msg_t *fsm_ipc_msg, struct fsm_request *fsm_req,
                 badge_t client_badge, bool *ret_with_cap)
{
        int ret, req, flags;
        struct fs_request *client_fs_req, *server_fs_req;
        ipc_msg_t *fs_ipc_msg;
        char path[FS_REQ_PATH_BUF_LEN];
        char server_path1[FS_REQ_PATH_BUF_LEN], server_path2[FS_REQ_PATH_BUF_LEN];
        struct mount_point_info_node *mpinfo1, *mpinfo2;
        int mount_id;
        bool path2;

        client_fs_req = &fsm_req->path_ops.fs_req;

        req = client_fs_req->req;

        /* Parse Flags */
        switch (req) {
        case FS_REQ_OPEN:
                flags = fs2MntFlag(client_fs_req->open.flags);
                break;
        case FS_REQ_CHMOD:
        case FS_REQ_CREAT:
        case FS_REQ_MKDIR:
        case FS_REQ_UNLINK:
        case FS_REQ_RMDIR:
        case FS_REQ_RENAME:
        case FS_REQ_FACCESSAT:
        case FS_REQ_SYMLINKAT:
                flags = MNT_READWRITE;
                break;
        default:
                flags = MNT_READONLY;
                break;
        }

        pthread_rwlock_rdlock(&fsm_client_namespace_table_rwlock);
        if (req == FS_REQ_SYMLINKAT && fsm_get_file_namespace(client_badge))
                ret = -EINVAL;
        else
                ret = 0;
        pthread_rwlock_unlock(&fsm_client_namespace_table_rwlock);
        if (ret < 0)
                goto out;

        /* Parse paths */
        switch (req) {
        case FS_REQ_CHMOD:
        case FS_REQ_OPEN:
        case FS_REQ_CREAT:
        case FS_REQ_MKDIR:
        case FS_REQ_UNLINK:
        case FS_REQ_RMDIR:
        case FS_REQ_STATFS:
        case FS_REQ_FSTATAT:
        case FS_REQ_FACCESSAT:
        case FS_REQ_SYMLINKAT:
        case FS_REQ_READLINKAT:
                path2 = false;
                strncpy(path, client_fs_req->path_ops.path, sizeof(path) - 1);
                path[sizeof(path) - 1] = '\0';
                ret = internal_parse_path(path, client_badge, flags, server_path1, sizeof(server_path1) - 1, &mpinfo1);
                break;
        case FS_REQ_RENAME:
                path2 = true;
                strncpy(path, client_fs_req->path_ops2.path1, sizeof(path) - 1);
                path[sizeof(path) - 1] = '\0';
                ret = internal_parse_path(path, client_badge, flags, server_path1, sizeof(server_path1) - 1, &mpinfo1);
                if (ret < 0)
                        goto out_path_ops2_fail;

                strncpy(path, client_fs_req->path_ops2.path2, sizeof(path) - 1);
                path[sizeof(path) - 1] = '\0';
                ret = internal_parse_path(path, client_badge, flags, server_path2, sizeof(server_path2) - 1, &mpinfo2);
                if (ret < 0)
                        goto out_path_ops2_fail;

                if (mpinfo1 != mpinfo2) {
                        ret = -EINVAL;
                        goto out_path_ops2_fail;
                }

        out_path_ops2_fail:
                break;
        default:
                /* not a path operation */
                ret = -EINVAL;
                break;
        }

        if (ret < 0)
                goto out;

        /* Send IPC to fs_server */
        fs_ipc_msg = ipc_create_msg(mpinfo1->_fs_ipc_struct, sizeof(struct fs_request));
        if (fs_ipc_msg == NULL) {
                ret = -EINVAL;
                goto out;
        }
        server_fs_req = (struct fs_request *)ipc_get_msg_data(fs_ipc_msg);

        /* XXX: fs_request struct is allocated twice relatively in client and
         * fsm */
        memcpy(server_fs_req, client_fs_req, sizeof(struct fs_request));
        if (path2) {
                server_fs_req->path_ops2.client_badge = client_badge;
                strncpy(server_fs_req->path_ops2.path1, server_path1, sizeof(server_fs_req->path_ops2.path1) - 1);
                server_fs_req->path_ops2.path1[sizeof(server_fs_req->path_ops2.path1) - 1] = '\0';
                strncpy(server_fs_req->path_ops2.path2, server_path2, sizeof(server_fs_req->path_ops2.path2) - 1);
                server_fs_req->path_ops2.path2[sizeof(server_fs_req->path_ops2.path2) - 1] = '\0';
        } else {
                server_fs_req->path_ops.client_badge = client_badge;
                strncpy(server_fs_req->path_ops.path, server_path1, sizeof(server_fs_req->path_ops.path) - 1);
                server_fs_req->path_ops.path[sizeof(server_fs_req->path_ops.path) - 1] = '\0';
        }

        ret = ipc_call(mpinfo1->_fs_ipc_struct, fs_ipc_msg);
        if (ret < 0)
                goto out_free;

        /* set response back to client's ipc_msg */
        switch (req) {
        case FS_REQ_OPEN:
                mount_id = get_mount_id(client_badge, mpinfo1, ret_with_cap);
                if (*ret_with_cap) {
                        ipc_set_msg_return_cap_num(fsm_ipc_msg, 1);
                        ipc_set_msg_cap(fsm_ipc_msg, 0, mpinfo1->fs_cap);
                }
                fsm_req->path_ops.mount_id = mount_id;
                fsm_req->path_ops.new_cap_flag = *ret_with_cap;
                break;
        case FS_REQ_STATFS:
                memcpy(&client_fs_req->stat.statfs, &server_fs_req->stat.statfs, sizeof(struct statfs));
                break;
        case FS_REQ_FSTATAT:
                memcpy(&client_fs_req->stat.stat, &server_fs_req->stat.stat, sizeof(struct stat));
                break;
        case FS_REQ_READLINKAT:
                if (ret > 0)
                        memcpy(client_fs_req->readlinkat.buf, server_fs_req->readlinkat.buf, ret);
        default:
                /* no response needs to be set back */
                break;
        }

out_free:
        ipc_destroy_msg(fs_ipc_msg);
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
        bool ret_with_cap = false;

        if (ipc_msg == NULL) {
                ipc_return(ipc_msg, -EINVAL);
        }

        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);
        switch (fsm_req->req) {
        case FSM_REQ_PATH_OPS: {
                ret = fsm_path_ops(
                        ipc_msg, fsm_req, client_badge, &ret_with_cap);
                break;
        }
        case FSM_REQ_MOUNT: {
                // path=(device_name), path2=(mount_point)
                ret = fsm_mount_fs(fsm_req->mount.path, fsm_req->mount.mount_path);
                break;
        }
        case FSM_REQ_UMOUNT: {
                ret = fsm_umount_fs(fsm_req->mount.path);
                break;
        }
        case FSM_REQ_SYNC: {
                ret = fsm_sync_page_cache();
                break;
        }
        case FSM_REQ_NEW_NAMESPACE: {
                struct user_file_namespace *fs_ns = NULL;
                if (fsm_req->new_ns.with_config != 0) {
                        fs_ns = &fsm_req->new_ns.fs_ns;
                }
                ret = fsm_new_file_namespace(fsm_req->new_ns.cur_badge,
                                             fsm_req->new_ns.parent_badge,
                                             fsm_req->new_ns.with_config,
                                             fs_ns);
                break;
        }
        case FSM_REQ_DELETE_NAMESPACE: {
                ret = fsm_delete_file_namespace(fsm_req->del_ns.badge);
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
