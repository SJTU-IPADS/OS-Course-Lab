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

#ifndef CHCORE_PORT_FS_CLIENT_DEFS_H
#define CHCORE_PORT_FS_CLIENT_DEFS_H

#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <assert.h>
#include <chcore/idman.h>
#include <pthread.h>

#include "fd.h"

/* This is statically linked to the `CLIENT process` */

#define MAX_CWD_LEN      255
#define MAX_CWD_BUF_LEN  MAX_CWD_LEN + 1
#define MAX_PATH_LEN     511
#define MAX_PATH_BUF_LEN MAX_PATH_LEN + 1

/* +++++++++++++++++++++++++ Client Metadata ++++++++++++++++++++++++++++++ */

/* current work directory (cwd) */
extern char cwd_path[MAX_CWD_BUF_LEN];
extern int cwd_len;

/* ++++++++++++++++++++++++ Client IPC Pool +++++++++++++++++++++++++++++++ */

#define MAX_MOUNT_ID 32
extern cap_t mounted_fs_cap[MAX_MOUNT_ID];
extern pthread_key_t mounted_fs_key;

ipc_struct_t *get_ipc_struct_by_mount_id(int mount_id);
void disconnect_mounted_fs(void);

/* ++++++++++++++++++++++++ File Descriptor Extension +++++++++++++++++++++ */

struct fd_record_extension {
        char path[MAX_PATH_LEN + 1];
        int mount_id;
};

/* Return new fd_record_extension struct */
struct fd_record_extension *_new_fd_record_extension();

/* ++++++++++++++++++++++++ Path Resolving && Utilities +++++++++++++++++++ */

char *path_from_fd(int fd);
int pathcpy(char *dst, size_t dst_buf_size, const char *path, size_t path_len);
char *path_join(const char *base, const char *next);
char *get_server_path(char *full_path, int full_len, char *mount_path,
                      int mount_len);

/* ++++++++++++++++++++++++ IPC Library ++++++++++++++++++++++++++++++++ */

struct fsm_request *fsm_parse_path_forward(ipc_msg_t *ipc_msg,
                                           const char *full_path);

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

void init_fs_client_side(void);

#endif /* CHCORE_PORT_FS_CLIENT_DEFS_H */
