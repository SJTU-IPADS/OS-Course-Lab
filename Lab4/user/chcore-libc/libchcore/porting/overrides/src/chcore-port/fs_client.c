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
#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <assert.h>
#include <chcore/idman.h>
#include <malloc.h>
#include <chcore/ipc.h>
#include <fcntl.h>
#include <string.h>
#include <chcore-internal/fs_debug.h>
#include <pthread.h>

#include "fs_client_defs.h"

/* CWD */
char cwd_path[MAX_CWD_BUF_LEN];
int cwd_len;

/* For client side mounted fs metadata */
cap_t mounted_fs_cap[MAX_MOUNT_ID] = {-1};
pthread_key_t mounted_fs_key;

ipc_struct_t *get_ipc_struct_by_mount_id(int mount_id)
{
        ipc_struct_t **mounted_fs_ipc_struct;
        ipc_struct_t *res;

        /* Get thread local  */
        mounted_fs_ipc_struct = pthread_getspecific(mounted_fs_key);
        if (!mounted_fs_ipc_struct) {
                mounted_fs_ipc_struct =
                        calloc(1, MAX_MOUNT_ID * sizeof(ipc_struct_t *));
                BUG_ON(!mounted_fs_ipc_struct);
                pthread_setspecific(mounted_fs_key, mounted_fs_ipc_struct);
        }

        /* Find thread local ipc struct, register one if it's not existed */
        if (!mounted_fs_ipc_struct[mount_id]) {
                mounted_fs_ipc_struct[mount_id] =
                        ipc_register_client(mounted_fs_cap[mount_id]);
                BUG_ON(!mounted_fs_ipc_struct[mount_id]);
        }

        res = mounted_fs_ipc_struct[mount_id];

        /* Do sanity check here. */
        BUG_ON(!res);
        return res;
}

void disconnect_mounted_fs(void)
{
        int i;
        ipc_struct_t **mounted_fs_ipc_struct;
        /* Get thread local  */
        mounted_fs_ipc_struct = pthread_getspecific(mounted_fs_key);
        if (!mounted_fs_ipc_struct) {
                return;
        }

        for (i = 0; i < MAX_MOUNT_ID; i++) {
                if (mounted_fs_ipc_struct[i]) {
                        ipc_client_close_connection(mounted_fs_ipc_struct[i]);
                }
        }

        free(mounted_fs_ipc_struct);
}

/*
 * Copy path to dst, dst_buf_size is dst's buffer size,
 * return: -1 means dst_buf_size is not large enough,
 *	  0 mean operation succeed.
 */
int pathcpy(char *dst, size_t dst_buf_size, const char *path, size_t path_len)
{
        if (dst_buf_size >= path_len + 1) {
                memset(dst, 0, dst_buf_size);
                strncpy(dst, path, path_len + 1);
        } else {
                return -1;
        }

        dst[path_len] = '\0';
        return 0;
}

/* ++++++++++++++++++++++++ File Descriptor Extension ++++++++++++++++++++++ */

struct fd_record_extension *_new_fd_record_extension(void)
{
        struct fd_record_extension *ret;
        ret = (struct fd_record_extension *)calloc(1, sizeof(*ret));
        if (ret == NULL) {
                return NULL;
        }
        ret->mount_id = -1;
        return ret;
}

/* ++++++++++++++++++++++++ Path Resolution ++++++++++++++++++++++++++++++++ */

char *path_from_fd(int fd)
{
        struct fd_record_extension *fd_ext;

        if (fd == AT_FDCWD)
                return cwd_path;
        else if (fd == AT_FDROOT)
                return "/";

        fd_ext = (struct fd_record_extension *)fd_dic[fd]->private_data;

        return fd_ext->path;
}

#define PATH_JOIN_SEPERATOR "/"

static bool __str_starts_with(const char *str, const char *start)
{
        for (;; str++, start++)
                if (!*start)
                        return true;
                else if (*str != *start)
                        return false;
}

static bool __str_ends_with(const char *str, const char *end)
{
        int end_len;
        int str_len;

        if (NULL == str || NULL == end)
                return false;

        end_len = strlen(end);
        str_len = strlen(str);

        return str_len < end_len ? false :
                                   !strcmp(str + str_len - end_len, end);
}

char *path_join(const char *dir, const char *file)
{
        char *filecopy;
        int size;
        char *buf;

        if (!dir)
                dir = "\0";
        if (!file)
                file = "\0";

        size = strlen(dir) + strlen(file) + 2;
        buf = malloc(size * sizeof(char));
        if (NULL == buf)
                return NULL;

        strcpy(buf, dir);

        /* add the sep if necessary */
        if (!__str_ends_with(dir, PATH_JOIN_SEPERATOR))
                strcat(buf, PATH_JOIN_SEPERATOR);

        /* remove the sep if necessary */
        if (__str_starts_with(file, PATH_JOIN_SEPERATOR)) {
                filecopy = strdup(file);
                if (NULL == filecopy) {
                        free(buf);
                        return NULL;
                }
                strcat(buf, ++filecopy);
                free(--filecopy);
        } else {
                strcat(buf, file);
        }

        return buf;
}

/* ++++++++++++++++++++++++++ IPC lib for FSM ++++++++++++++++++++++++++++++ */

/*
 * ipc_msg is FSM ipc message, and should be created outside
 * return: fsm_req = ipc_msg_data(ipc_msg)
 *	 fsm_req->mount_id = corresponding fs_server mount_id
 *	 fsm_req->mount_path/path_len = corresponding fs_server mount_point
 */
struct fsm_request *fsm_parse_path_forward(ipc_msg_t *ipc_msg,
                                           const char *full_path)
{
        struct fsm_request *fsm_req;
        int mount_id;
        int ret;

        fsm_req = (struct fsm_request *)ipc_get_msg_data(ipc_msg);

        /* Send ipc to FSM */
        fsm_req->req = FSM_REQ_PARSE_PATH;
        if (pathcpy(fsm_req->path,
                    FS_REQ_PATH_BUF_LEN,
                    full_path,
                    strlen(full_path))
            != 0)
                return NULL;

        BUG_ON(!fsm_ipc_struct);
        ret = ipc_call(fsm_ipc_struct, ipc_msg);
        if (ret < 0) {
                return NULL;
        }

        /* Bind mount_id and cap. */
        mount_id = fsm_req->mount_id;
        if (fsm_req->new_cap_flag) {
                assert(mount_id >= 0 && mount_id < MAX_MOUNT_ID);
                mounted_fs_cap[mount_id] = ipc_get_msg_cap(ipc_msg, 0);
        }

        assert(mounted_fs_cap[mount_id] > 0);

        /* Call this for initialize tls ipc data, and we don't need return value
         */
        get_ipc_struct_by_mount_id(mount_id);

        return fsm_req;
}

/* ++++++++++++++++++++++++++++++++ Others +++++++++++++++++++++++++++++++++ */

void init_fs_client_side(void)
{
        pthread_key_create(&mounted_fs_key, NULL);

        /* Initialize cwd as ROOT */
        cwd_path[0] = '/';
        cwd_path[1] = '\0';
        cwd_len = 1;
}
