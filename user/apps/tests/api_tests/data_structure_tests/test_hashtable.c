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

#include <chcore/container/hashtable.h>
#include <stdio.h>
#include <assert.h>

struct htable_node {
        struct hlist_node node;
        unsigned int key;
        int value;
};

int main()
{
        struct htable ht;
        struct htable_node *hn1 = malloc(sizeof(struct htable_node));
        struct htable_node *hn2 = malloc(sizeof(struct htable_node));
        struct htable_node *hn3;
        struct htable_node *hn4 = malloc(sizeof(struct htable_node));
        init_htable(&ht, 5);
        printf("init_htable success.\n");
        if (htable_empty(&ht))
                printf("List is empty.\n");
        else {
                printf("List is not empty!\n");
                return -1;
        }
        init_hlist_node(&hn1->node);
        init_hlist_node(&hn2->node);
        init_hlist_node(&hn4->node);
        hn1->key = 1;
        hn1->value = 4;
        htable_add(&ht, hn1->key, &hn1->node);
        hn2->key = 2;
        hn2->value = 3;
        htable_add(&ht, hn2->key, &hn2->node);
        hn4->key = 4;
        hn4->value = 1;
        htable_add(&ht, hn4->key, &hn4->node);
        printf("htable_add success.\n");
        if (!htable_empty(&ht))
                printf("Htable is not empty.\n");
        else {
                printf("Htable is empty!\n");
                return -1;
        }
        struct hlist_head *hlist_node = htable_get_bucket(&ht, 1);
        assert(hlist_node);
        for_each_in_hlist(hn3, node, hlist_node) {
                printf("Key 1 node's value is %d\n", hn3->value);
        }
        printf("htable_get_bucket success.\n");
        htable_del(&hn2->node);
        printf("htable_del success.\n");
        htable_free(&ht);
        printf("htable_free success.\n");
        printf("Test finish!\n");
        return 0;
}