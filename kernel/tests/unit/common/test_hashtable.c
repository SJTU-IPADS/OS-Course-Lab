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

void printk(const char *fmt, ...){};
#include <common/errno.h>
#include <common/macro.h>
#include "test_redef.h"

#include "hashtable.h"

#include "test.h"

MU_TEST(test_htable)
{
        struct htable htable;
        struct dummy_data *data;
        struct dummy_data *dump;
        struct dummy_data *iter;
        struct dummy_data *tmp;
        int i;
        int b;

        srand(RND_SEED);

        init_htable(&htable, 1024);

        data = malloc(sizeof(*data) * RND_NODES);

        /* Insert nodes to the hlist. */
        for (i = 0; i < RND_NODES; ++i) {
                data[i].index = i;
                data[i].key = (u32)rand_value(VALUE_MAX);
                data[i].value = (u32)rand_value(VALUE_MAX);
                htable_add(&htable, data[i].key, &data[i].hnode);
                printf("i=%d key=%d value=%d\n", i, data[i].key, data[i].value);
        }

        dump = malloc(sizeof(*dump) * RND_NODES);
        /* Traverse items. */
        i = 0;
        for_each_in_htable (iter, b, hnode, &htable) {
                dump[i] = *iter;
                printf("traverse: i=%d key=%d value=%d\n",
                       i,
                       dump[i].key,
                       dump[i].value);
                ++i;
        }

        /* Check. */
        qsort(dump, RND_NODES, sizeof(*dump), compare_index);

        for (i = 0; i < RND_NODES; ++i) {
                if (data[i].key != dump[i].key
                    || data[i].value != dump[i].value)
                        printf("i=%d key=%d %d value=%d %d\n",
                               i,
                               data[i].key,
                               dump[i].key,
                               data[i].value,
                               dump[i].value);
                mu_check(data[i].key == dump[i].key);
                mu_check(data[i].value == dump[i].value);
        }

        /* Traverse again and remove items. */
        for_each_in_htable_safe (iter, tmp, b, hnode, &htable) {
                printf("del key=%d value=%d\n", iter->key, iter->value);
                htable_del(&iter->hnode);
        }

        /* Traverse an empty list. */
        for_each_in_htable (iter, b, hnode, &htable) {
                printf("remain key=%d value=%d\n", iter->key, iter->value);
                mu_check(false);
        }
        mu_check(htable_empty(&htable));
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_htable);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
