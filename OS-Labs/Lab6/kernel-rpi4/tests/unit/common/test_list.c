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

#include <stdlib.h>
#include <minunit.h>
#include "test_redef.h"

#include "list.h"

#include "test.h"

MU_TEST(test_hlist)
{
        struct hlist_head head;
        struct dummy_data *data;
        struct dummy_data *dump;
        struct dummy_data *iter;
        struct dummy_data *tmp;
        int i;

        srand(RND_SEED);

        init_hlist_head(&head);

        mu_check(hlist_empty(&head));

        data = malloc(sizeof(*data) * RND_NODES);
        dump = malloc(sizeof(*dump) * RND_NODES);

        /* Insert nodes to the list. */
        for (i = 0; i < RND_NODES; ++i) {
                data[i].index = i;
                data[i].key = 0;
                data[i].value = rand_value(VALUE_MAX);
                init_hlist_node(&data[i].hnode);
                hlist_add(&data[i].hnode, &head);
                dump[i] = data[i];
        }

        mu_check(!hlist_empty(&head));

        /* Traverse. */
        i = RND_NODES;
        for_each_in_hlist (iter, hnode, &head) {
                i--;
                printf("traverse: %d %p->%p\n",
                       iter->value,
                       iter,
                       iter->hnode.next);
                mu_check(iter->value == data[i].value);
        }

        /* Traverse and remove one by one. */
        i = RND_NODES;
        for_each_in_hlist_safe (iter, tmp, hnode, &head) {
                i--;
                printf("to delete: %d iter->iternext: %p->%p "
                       "tmp->tmpnext: %p->%p\n",
                       iter->value,
                       iter,
                       iter->hnode.next,
                       tmp,
                       tmp ? tmp->hnode.next : NULL);
                mu_check(iter->value == data[i].value);
                hlist_del(&iter->hnode);
        }

        mu_check(hlist_empty(&head));

        /* Traverse an empty list. */
        for_each_in_hlist (iter, hnode, &head) {
                mu_check(false);
        }

        /* Insert nodes to the list again. */
        for (i = 0; i < RND_NODES; ++i) {
                init_hlist_node(&data[i].hnode);
                hlist_add(&data[i].hnode, &head);
        }
        printf("list after second insertion\n");
        kprint_hlist(&head);

        /* Remove in random order */
        qsort(dump, RND_NODES, sizeof(*dump), compare_kv);
        for (i = 0; i < RND_NODES; ++i)
                hlist_del(&data[dump[i].index].hnode);

        mu_check(hlist_empty(&head));
}

MU_TEST(test_list)
{
        struct list_head head;
        struct dummy_data *data;
        struct dummy_data *iter;
        struct dummy_data *tmp;
        int i;

        srand(RND_SEED);

        init_list_head(&head);

        mu_check(list_empty(&head));

        data = malloc(sizeof(*data) * RND_NODES);

        /* Insert nodes to the list. */
        for (i = 0; i < RND_NODES; ++i) {
                data[i].value = rand_value(VALUE_MAX);
                list_add(&data[i].node, &head);
        }

        mu_check(!list_empty(&head));

        /* Traverse. */
        i = RND_NODES;
        for_each_in_list (iter, struct dummy_data, node, &head) {
                i--;
                mu_check(iter->value == data[i].value);
        }

        /* Traverse and remove one by one. */
        i = RND_NODES;
        for_each_in_list_safe (iter, tmp, node, &head) {
                i--;
                mu_check(iter->value == data[i].value);
                list_del(&iter->node);
        }

        mu_check(list_empty(&head));

        /* Append nodes to the list. */
        for (i = 0; i < RND_NODES; ++i) {
                data[i].value = rand_value(VALUE_MAX);
                list_append(&data[i].node, &head);
        }

        /* Traverse again and remove items. */
        i = 0;
        for_each_in_list_safe (iter, tmp, node, &head) {
                mu_check(iter->value == data[i].value);
                list_del(&iter->node);
                i++;
        }

        /* Traverse an empty list. */
        for_each_in_list (iter, struct dummy_data, node, &head) {
                mu_check(false);
        }
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_list);
        MU_RUN_TEST(test_hlist);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
