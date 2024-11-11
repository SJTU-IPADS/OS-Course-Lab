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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <minunit.h>

#include "../../porting/overrides/src/chcore-port/rbtree.c"

#include "../../porting/overrides/src/chcore-port/rbtree_plus.c"

struct test_node {
        struct rbp_node node;
        int key;
        int val;
};

static int comp_key_test_node(const void *key, const struct rbp_node *rbp_node)
{
        const struct test_node *test_node =
                container_of(rbp_node, struct test_node, node);
        const int int_key = *(int *)key;

        if (int_key < test_node->key)
                return -1;
        else if (int_key > test_node->key)
                return 1;
        else
                return 0;
}

static bool less_test_node(const struct rbp_node *a, const struct rbp_node *b)
{
        const struct test_node *test_node_a =
                container_of(a, struct test_node, node);
        const struct test_node *test_node_b =
                container_of(b, struct test_node, node);

        return test_node_a->key < test_node_b->key;
}

static int keys[] = {7, 2, 4, 9, 5, 0, 1, 3, 6, 8};

MU_TEST(basic_test)
{
        struct rbtree_plus *tree = malloc(sizeof(*tree));
        init_rbtree_plus(tree);
        mu_check(rbp_first(tree) == NULL);
        mu_check(rbp_last(tree) == NULL);

        for (int i = 0; i < 10; i++) {
                struct test_node *node = malloc(sizeof(*node));
                node->key = keys[i];
                node->val = i;
                rbp_insert(tree, &node->node, less_test_node);
        }

        for (int i = 0; i < 10; i++) {
                struct rbp_node *ret =
                        rbp_search(tree, &keys[i], comp_key_test_node);
                mu_check(ret);
                struct test_node *test_node =
                        container_of(ret, struct test_node, node);
                mu_check(test_node->val == i);
        }

        struct rbp_node *node, *tmp;
        struct test_node *iter, *prev = NULL;
        rbp_for_each(tree, node)
        {
                iter = container_of(node, struct test_node, node);
                if (prev) {
                        mu_check(prev->key <= iter->key);
                }
                prev = iter;
        }

        rbp_for_each_safe(tree, node, tmp)
        {
                iter = container_of(node, struct test_node, node);
                rbp_erase(tree, node);
                free(iter);
        }
}

#define TEST_NUM 1000000

MU_TEST(random_test) {
        struct rbtree_plus *tree = malloc(sizeof(*tree));
        init_rbtree_plus(tree);
        srand(time(NULL));
        
        for (int i = 0; i < TEST_NUM; i++) {
                struct test_node *node = malloc(sizeof(*node));
                node->key = rand();
                node->val = i;
                rbp_insert(tree, &node->node, less_test_node);
        }

        struct rbp_node *node, *tmp;
        struct test_node *iter, *prev = NULL;
        rbp_for_each(tree, node)
        {
                iter = container_of(node, struct test_node, node);
                if (prev) {
                        mu_check(prev->key <= iter->key);
                }
                prev = iter;
        }

        rbp_for_each_safe(tree, node, tmp)
        {
                iter = container_of(node, struct test_node, node);
                if (iter->val % 2 == 0) {
                        rbp_erase(tree, node);
                        free(iter);
                }
        }

        prev = NULL;

        rbp_for_each(tree, node)
        {
                iter = container_of(node, struct test_node, node);
                if (prev) {
                        mu_check(prev->key <= iter->key);
                }
                prev = iter;
        }

        rbp_for_each_safe(tree, node, tmp)
        {
                iter = container_of(node, struct test_node, node);
                rbp_erase(tree, node);
                free(iter);
        }
}

int main()
{
        MU_RUN_TEST(basic_test);
        MU_RUN_TEST(random_test);
        MU_REPORT();
        return minunit_status;
}