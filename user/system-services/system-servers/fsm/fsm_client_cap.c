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

#include <malloc.h>
#include <string.h>
#include "fsm_client_cap.h"
#include <errno.h>

struct list_head fsm_client_cap_table;

/* Return mount_id */
int fsm_set_client_cap(badge_t client_badge, cap_t cap)
{
        struct fsm_client_cap_node *n;

        for_each_in_list (
                n, struct fsm_client_cap_node, node, &fsm_client_cap_table) {
                if (n->client_badge == client_badge) {
                        /* Client already visited */
                        BUG_ON(n->cap_num >= 16);
                        n->cap_table[n->cap_num] = cap;
                        n->cap_num++;
                        return n->cap_num - 1;
                }
        }

        /* Client is not visited, create a new fsm_client_cap_node */
        n = (struct fsm_client_cap_node *)malloc(sizeof(*n));
        if (n == NULL) {
                return -ENOMEM;
        }
        n->client_badge = client_badge;
        memset(n->cap_table, 0, sizeof(n->cap_table));
        n->cap_table[0] = cap;
        n->cap_num = 1;

        list_append(&n->node, &fsm_client_cap_table);

        return 0;
}

/* Return mount_id if record exists, otherwise -1 */
int fsm_get_client_cap(badge_t client_badge, cap_t cap)
{
        struct fsm_client_cap_node *n;
        int i;

        for_each_in_list (
                n, struct fsm_client_cap_node, node, &fsm_client_cap_table)
                if (n->client_badge == client_badge)
                        for (i = 0; i < n->cap_num; i++)
                                if (n->cap_table[i] == cap)
                                        return i;

        return -1;
}
