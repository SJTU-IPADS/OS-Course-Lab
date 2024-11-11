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

#ifndef FSM_FILE_NAMESPACE_H
#define FSM_FILE_NAMESPACE_H

#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include "defs.h"

/* ------------------------------------------------------------------------ */

#define MNT_READONLY	(0x00)
#define MNT_READWRITE	(0x01)

/*
 * Permission Informations
 */

struct file_namespace {
        struct list_head file_infos;
        int refcount;
        pthread_rwlock_t file_infos_rwlock;
};

extern struct list_head fsm_client_namespace_table;
extern pthread_rwlock_t fsm_client_namespace_table_rwlock;

/* Caller should fetch `fsm_client_namespace_table_rwlock` read lock before call */
struct file_namespace *fsm_get_file_namespace(badge_t client_badge);

/* Caller should fetch `fsm_client_namespace_table_rwlock` write lock before call */
int fsm_new_file_namespace(badge_t client_badge, badge_t parent_badge,
                           int with_config, struct user_file_namespace *fs_ns);
int fsm_delete_file_namespace(badge_t client_badge);

int parse_path_from_file_namespace(struct file_namespace *file_ns, const char *path,
                                   int flags, char *full_path,
                                   size_t full_path_len);

int fs2MntFlag(int fs_flags);

#endif /* FSM_FILE_NAMESPACE_H */
