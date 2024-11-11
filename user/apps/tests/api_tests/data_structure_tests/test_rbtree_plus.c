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

#include <chcore/container/rbtree_plus.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct tree_node {
        struct rbp_node node;
        int key;
        int value;
};

bool tree_less(const struct rbp_node *lhs, const struct rbp_node *rhs)
{
        struct tree_node *t_lhs = container_of(lhs, struct tree_node, node),
                         *t_rhs = container_of(rhs, struct tree_node, node);
        return t_lhs->key < t_rhs->key;
}

int tree_cmp_key(const void *key, const struct rbp_node *node)
{
        int node_key = container_of(node, struct tree_node, node)->key;
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
        struct rbtree_plus root;
        struct tree_node *tn1 = malloc(sizeof(struct tree_node));
        struct tree_node *tn2 = malloc(sizeof(struct tree_node));
        struct tree_node *tn3 = malloc(sizeof(struct tree_node));
        struct tree_node *tn4 = malloc(sizeof(struct tree_node));
        struct tree_node *tn5 = malloc(sizeof(struct tree_node));
        int key;
        
        init_rbtree_plus(&root);
        printf("init_rbtree_plus success.\n");
        
        tn1->key = 1;
        tn1->value = 4;
        rbp_insert(&root, &tn1->node, tree_less);
        tn2->key = 2;
        tn2->value = 3;
        rbp_insert(&root, &tn2->node, tree_less);
        tn3->key = 3;
        tn3->value = 2;
        rbp_insert(&root, &tn3->node, tree_less);
        tn4->key = 2;
        tn4->value = 1;
        rbp_insert(&root, &tn4->node, tree_less);
        printf("rbp_insert success.\n");
        
        key = 2;
        struct rbp_node *first_node = rbp_search_first(&root, &key, tree_cmp_key);
        assert(first_node);
        struct tree_node *tree_node = container_of(first_node, struct tree_node, node);
        printf("First key 2 node's value is %d\n", tree_node->value);
        printf("rbp_search_first success.\n");
        key = 4;
        first_node = rbp_search_nearest(&root, &key, tree_cmp_key);
        assert(first_node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Key 4 nearest node's value is %d\n", tree_node->value);
        key = 1;
        first_node = rbp_search_nearest(&root, &key, tree_cmp_key);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Key 1 nearest node's value is %d\n", tree_node->value);
        printf("rbp_search_nearest success.\n");
        key = 3;
        first_node = rbp_search(&root, &key, tree_cmp_key);
        assert(first_node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Key 3 node's value is %d\n", tree_node->value);
        printf("rbp_search success.\n");
        
        tn5->key = 2;
        tn5->value = 5;
        rbp_replace_node(&root, &tn4->node, &tn5->node);
        printf("rbp_replace_node success.\n");

        struct rbp_node *node = rbp_next(&root, first_node);
        assert(!node);
        printf("rbp_next success.\n");
        node = rbp_prev(&root, first_node);
        assert(node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Prev node's value is %d\n", tree_node->value);
        printf("rbp_prev success.\n");
        node = rbp_last(&root);
        assert(node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Last node's value is %d\n", tree_node->value);
        printf("rbp_last success.\n");
        node = rbp_first(&root);
        assert(node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("First node's value is %d\n", tree_node->value);
        printf("rbp_first success.\n");

        first_node = rbp_next_match(&root, node, &key, tree_cmp_key);
        assert(!first_node);
        key = 2;
        first_node = rbp_next_match(&root, node, &key, tree_cmp_key);
        assert(first_node);
        tree_node = container_of(first_node, struct tree_node, node);
        printf("Next match node's value is %d\n", tree_node->value);
        printf("rbp_next_match success.\n");

        rbp_erase(&root, &tn4->node);
        rbp_erase(&root, &tn3->node);
        rbp_erase(&root, &tn2->node);
        rbp_erase(&root, &tn1->node);
        printf("rb_erase success.\n");

        free(tn4);
        free(tn3);
        free(tn2);
        free(tn1);
        printf("Test finish!\n");
        return 0;
}