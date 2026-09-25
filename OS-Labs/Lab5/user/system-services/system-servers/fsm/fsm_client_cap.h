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

#ifndef FSM_CLIENT_CAP_H
#define FSM_CLIENT_CAP_H

#include <chcore-internal/fs_defs.h>
#include <chcore/container/list.h>
#include <pthread.h>

/* ------------------------------------------------------------------------ */

/*
 * FSM will record all the caps of fs those are sended to some client.
 * Such information is recorded in the following structure.
 */
struct fsm_client_cap_node {
        badge_t client_badge;

        cap_t cap_table[16];
        int cap_num;

        struct list_head node;
};

extern struct list_head fsm_client_cap_table;
extern pthread_mutex_t fsm_client_cap_table_lock;

int fsm_set_client_cap(badge_t client_badge, cap_t cap);
int fsm_get_client_cap(badge_t client_badge, cap_t cap);

#endif /* FSM_CLIENT_CAP_H */