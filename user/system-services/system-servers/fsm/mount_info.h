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

#ifndef FSM_MOUNT_INFO_H
#define FSM_MOUNT_INFO_H

#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include "defs.h"

/* ------------------------------------------------------------------------ */

/*
 * Mount Point Informations
 */
struct mount_point_info_node {
        cap_t fs_cap;
        char path[MAX_MOUNT_POINT_LEN + 1];
        int path_len;
        ipc_struct_t *_fs_ipc_struct;
        int refcnt;

        struct list_head node;
};

extern int fs_num;

extern struct list_head mount_point_infos;
extern pthread_rwlock_t mount_point_infos_rwlock;

struct mount_point_info_node *set_mount_point(const char *path, int path_len,
                                              int fs_cap);
struct mount_point_info_node *get_mount_point(const char *path, int path_len);
int remove_mount_point(char *path);

#endif /* FSM_MOUNT_INFO_H */
