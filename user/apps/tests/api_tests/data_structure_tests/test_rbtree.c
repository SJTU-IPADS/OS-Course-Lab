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

#include <chcore/container/rbtree.h>
#include <chcore/container/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct tree_node {
        struct rb_node node;
        int key;
        int value;
};

bool tree_less(const struct rb_node *lhs, const struct rb_node *rhs)
{
        struct tree_node *t_lhs = rb_entry(lhs, struct tree_node, node),
                         *t_rhs = rb_entry(rhs, struct tree_node, node);
        return t_lhs->key < t_rhs->key;
}

int tree_cmp_key(const void *key, const struct rb_node *node)
{
        int node_key = rb_entry(node, struct tree_node, node)->key;
        int ikey = *((int*)key);
        if (ikey < node_key) {
                return -1;
        } else if (ikey > node_key) {
                return 1;
        } else {
                return 0;
        }
}

int main()
{
        struct rb_root root;
        struct tree_node *tn1 = malloc(sizeof(struct tree_node));
        struct tree_node *tn2 = malloc(sizeof(struct tree_node));
        struct tree_node *tn3 = malloc(sizeof(struct tree_node));
        struct tree_node *tn4 = malloc(sizeof(struct tree_node));
        struct tree_node *tn5 = malloc(sizeof(struct tree_node));
        int key;
        
        init_rb_root(&root);
        printf("init_rb_root success.\n");
        
        tn1->key = 1;
        tn1->value = 4;
        rb_insert(&root, &tn1->node, tree_less);
        tn2->key = 2;
        tn2->value = 3;
        rb_insert(&root, &tn2->node, tree_less);
        tn3->key = 3;
        tn3->value = 2;
        rb_insert(&root, &tn3->node, tree_less);
        tn4->key = 2;
        tn4->value = 1;
        rb_insert(&root, &tn4->node, tree_less);
        printf("rb_insert success.\n");
        
        key = 2;
        struct rb_node *first_node = rb_search_first(&root, &key, tree_cmp_key);
        assert(first_node);
        struct tree_node *tree_node = rb_entry(first_node, struct tree_node, node);
        printf("First key 2 node's value is %d\n", tree_node->value);
        printf("rb_search_first success.\n");
        key = 3;
        first_node = rb_search(&root, &key, tree_cmp_key);
        assert(first_node);
        tree_node = rb_entry(first_node, struct tree_node, node);
        printf("Key 3 node's value is %d\n", tree_node->value);
        printf("rb_search success.\n");
        
        tn5->key = 2;
        tn5->value = 5;
        rb_replace_node(&root, &tn4->node, &tn5->node);
        printf("rb_replace_node success.\n");

        struct rb_node *node = rb_next(first_node);
        assert(!node);
        printf("rb_next success.\n");
        node = rb_prev(first_node);
        assert(node);
        tree_node = rb_entry(first_node, struct tree_node, node);
        printf("Prev node's value is %d\n", tree_node->value);
        printf("rb_prev success.\n");
        node = rb_last(&root);
        assert(node);
        tree_node = rb_entry(first_node, struct tree_node, node);
        printf("Last node's value is %d\n", tree_node->value);
        printf("rb_last success.\n");
        node = rb_first(&root);
        assert(node);
        tree_node = rb_entry(first_node, struct tree_node, node);
        printf("First node's value is %d\n", tree_node->value);
        printf("rb_first success.\n");

        first_node = rb_next_match(node, &key, tree_cmp_key);
        assert(!first_node);
        key = 2;
        first_node = rb_next_match(node, &key, tree_cmp_key);
        assert(first_node);
        tree_node = rb_entry(first_node, struct tree_node, node);
        printf("Next match node's value is %d\n", tree_node->value);
        printf("rb_next_match success.\n");

        rb_erase(&root, &tn4->node);
        rb_erase(&root, &tn3->node);
        rb_erase(&root, &tn2->node);
        rb_erase(&root, &tn1->node);
        printf("rb_erase success.\n");

        free(tn4);
        free(tn3);
        free(tn2);
        free(tn1);
        printf("Test finish!\n");
        return 0;
}