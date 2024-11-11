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

#ifndef CHANMGR_H
#define CHANMGR_H

#include <chcore/ipc.h>
#include <chcore-internal/chanmgr_defs.h>
#include <pthread.h>
#include <chcore/container/hashtable.h>

#define GTASK_PID    (7)
#define GTASK_TID    (0xa)
#define GTASK_TASKID (pid_to_taskid(GTASK_TID, GTASK_PID))

struct chanmgr {
        struct htable name2chan;
        struct htable cid2chan;
        struct htable badge2chan;
        pthread_mutex_t lock;
};

int chanmgr_init(void);
void chanmgr_deinit(void);

/*
 * chanmgr ipc interfaces for channel registering, indexing, deregistering
 */
void chanmgr_handle_create_channel(ipc_msg_t *ipc_msg, badge_t badge, int pid,
                                   int tid);
void chanmgr_handle_remove_channel(ipc_msg_t *ipc_msg, badge_t badge, int pid,
                                   int tid);
void chanmgr_handle_hunt_by_name(ipc_msg_t *ipc_msg, int pid, int tid);
void chanmgr_handle_get_ch_from_path(ipc_msg_t *ipc_msg, int pid, int tid);
void chanmgr_handle_get_ch_from_taskid(ipc_msg_t *ipc_msg, int pid, int tid);

/*
 * Destructor called when client cap_group exiting. Destructor will free
 * corresponding channel records.
 */
void chanmgr_destructor(badge_t client_badge);

#endif /* CHANMGR_H */