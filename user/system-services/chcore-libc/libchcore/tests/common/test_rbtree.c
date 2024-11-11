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

#include <assert.h>
#include <stdlib.h>
#include <minunit.h>
#include "../../porting/overrides/src/chcore-port/rbtree.c"
#include <chcore/container/list.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#define TEST_ROUNDS (100000)

struct answer_node {
        struct list_head node;
        int key;
        int value;
        struct list_head** slot;
};

struct answer_struct {
        int answer_cnt;
        struct list_head** answer_slots;
        struct list_head answers_head;
};

struct tree_node {
        struct rb_node node;
        int key;
        int value;
};

struct answer_struct* answer_struct_new(int ans_nr)
{
        struct answer_struct* ret = calloc(1, sizeof(struct answer_struct));
        init_list_head(&ret->answers_head);
        ret->answer_slots = malloc(sizeof(struct list_head*) * ans_nr);
        return ret;
}

bool answer_struct_insert(struct answer_struct* this, int key, int value)
{
        if (this->answer_slots[key])
                return false;
        struct answer_node* node = malloc(sizeof(struct answer_node));
        node->key = key;
        node->value = value;
        node->slot = &this->answer_slots[key];
        list_append(&node->node, &this->answers_head);
        this->answer_cnt++;
        this->answer_slots[key] = &node->node;
        return true;
}

struct answer_node* answer_struct_get(struct answer_struct* this, int key)
{
        if (!this->answer_slots[key]) {
                return NULL;
        } else {
                return container_of(
                        this->answer_slots[key], struct answer_node, node);
        }
}

void answer_struct_erase(struct answer_struct* this, int key)
{
        if (!this->answer_slots[key])
                return;

        struct answer_node* node =
                container_of(this->answer_slots[key], struct answer_node, node);
        list_del(&node->node);
        this->answer_slots[key] = NULL;
        this->answer_cnt--;
        free(node);
}

int rb_blackheight(struct rb_node* node)
{
        if (!node) {
                return 0;
        }
        int lhs = rb_blackheight(node->left_child);
        int rhs = rb_blackheight(node->right_child);
        assert(lhs == rhs);
        if (rb_is_red(node)) {
                assert(!node->left_child || rb_is_black(node->left_child));
                assert(!node->right_child || rb_is_black(node->right_child));
                return lhs;
        } else if (rb_is_black(node)) {
                return lhs + 1;
        }
        assert(0);
}

bool rb_is_rbtree(struct rb_root* root)
{
        struct rb_node* root_node = root->root_node;

        if (!root_node) {
                return true;
        }

        if (rb_is_red(root_node)) {
                return false;
        }

        return rb_blackheight(root_node->left_child)
               == rb_blackheight(root_node->right_child);
}

bool tree_less(const struct rb_node* lhs, const struct rb_node* rhs)
{
        struct tree_node *t_lhs = rb_entry(lhs, struct tree_node, node),
                         *t_rhs = rb_entry(rhs, struct tree_node, node);
        return t_lhs->key < t_rhs->key;
}

int tree_cmp_node(const struct rb_node* lhs, const struct rb_node* rhs)
{
        struct tree_node *t_lhs = rb_entry(lhs, struct tree_node, node),
                         *t_rhs = rb_entry(rhs, struct tree_node, node);
        int lkey = t_lhs->key, rkey = t_rhs->key;
        if (lkey < rkey) {
                return -1;
        } else if (lkey > rkey) {
                return 1;
        } else {
                return 0;
        }
}

int tree_cmp_key(const void* key, const struct rb_node* node)
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

struct traced_rand {
        FILE* trace;
};

struct traced_rand* traced_rand_new(char* filename)
{
        FILE* f = fopen(filename, "a+");
        struct traced_rand* t = malloc(sizeof(struct traced_rand));
        t->trace = f;
        srand(time(NULL));
        return t;
}

int traced_rand_kv(struct traced_rand* this, int* key, int* value)
{
        int op, k, v, ret;
        ret = fscanf(this->trace, "%d %d %d", &op, &k, &v);
        if (ret == EOF) {
                op = rand() % 6;
                k = rand() % TEST_ROUNDS;
                v = rand();
                fprintf(this->trace, "%d %d %d\n", op, k, v);
                fflush(this->trace);
        } else if (ret < 0) {
                assert(0);
        }
        *key = k;
        *value = v;
        return op;
}

struct traced_rand* tracer = NULL;

MU_TEST(test_fuzzing_rbtree)
{
        struct rb_root* tree = malloc(sizeof(struct rb_root));
        struct answer_struct* answers = answer_struct_new(TEST_ROUNDS);
        init_rb_root(tree);

        for (int i = 0; i < TEST_ROUNDS; i++) {
                mu_check(rb_is_rbtree(tree));

                int op, key, val;
                op = traced_rand_kv(tracer, &key, &val);
                switch (op) {
                case 0: {
                        // test insert
                        if (!answer_struct_insert(answers, key, val)) {
                                continue;
                        } else {
                                struct tree_node* tn =
                                        malloc(sizeof(struct tree_node));
                                tn->key = key;
                                tn->value = val;
                                rb_insert(tree, &tn->node, tree_less);
                        }
                        break;
                }
                case 1: {
                        // test random search(including non-exist key)
                        struct answer_node* answer =
                                answer_struct_get(answers, key);
                        struct rb_node* rnode =
                                rb_search(tree, &key, tree_cmp_key);
                        if (answer) {
                                struct tree_node* tree_node =
                                        rb_entry(rnode, struct tree_node, node);
                                mu_check(rnode);
                                mu_check(tree_node->value == answer->value);
                        } else {
                                mu_check(!rnode);
                        }
                        break;
                }
                case 2: {
                        if (answers->answer_cnt <= 0) {
                                continue;
                        }
                        // test search existing key
                        int key_idx = val % answers->answer_cnt, i = 0;
                        struct answer_node* answer;
                        for_each_in_list (answer,
                                          struct answer_node,
                                          node,
                                          &answers->answers_head) {
                                if (i == key_idx) {
                                        break;
                                }
                                i++;
                        }
                        struct rb_node* r =
                                rb_search(tree, &answer->key, tree_cmp_key);
                        mu_check(r);
                        struct tree_node* tree_node =
                                rb_entry(r, struct tree_node, node);
                        mu_check(tree_node->key == answer->key);
                        mu_check(tree_node->value == answer->value);
                        break;
                }
                case 3: {
                        if (answers->answer_cnt <= 0) {
                                continue;
                        }
                        // test search and erase existing key
                        int key_idx = val % answers->answer_cnt, i = 0;
                        struct answer_node* answer;
                        for_each_in_list (answer,
                                          struct answer_node,
                                          node,
                                          &answers->answers_head) {
                                if (i == key_idx) {
                                        break;
                                }
                                i++;
                        }
                        struct rb_node* r =
                                rb_search(tree, &answer->key, tree_cmp_key);
                        mu_check(r);

                        rb_erase(tree, r);
                        r = rb_search(tree, &answer->key, tree_cmp_key);
                        mu_check(!r);

                        answer_struct_erase(answers, answer->key);
                        break;
                }
                case 4: {
                        // test traversal
                        if (answers->answer_cnt <= 2) {
                                continue;
                        }

                        struct rb_node *r1, *r2;
                        struct tree_node *t1, *t2;
                        for (r1 = rb_first(tree), r2 = rb_next(r1); r2;
                             r1 = r2, r2 = rb_next(r2)) {
                                t1 = rb_entry(r1, struct tree_node, node);
                                t2 = rb_entry(r2, struct tree_node, node);
                                mu_check(t1->key <= t2->key);
                        }
                        break;
                }
                case 5: {
                        // test reverse traversal
                        if (answers->answer_cnt <= 2) {
                                continue;
                        }

                        struct rb_node *r1, *r2;
                        struct tree_node *t1, *t2;
                        for (r1 = rb_last(tree), r2 = rb_prev(r1); r2;
                             r1 = r2, r2 = rb_prev(r2)) {
                                t1 = rb_entry(r1, struct tree_node, node);
                                t2 = rb_entry(r2, struct tree_node, node);
                                mu_check(t1->key >= t2->key);
                        }
                        break;
                }
                default:
                        BUG("Unsupported op type");
                        break;
                }
        }
}

#define TEST_INSERT_KEYS (1000000)
#define TEST_ERASE_KEYS  (500000)

MU_TEST(test_batch_processing)
{
        struct answer_struct* answers = answer_struct_new(TEST_INSERT_KEYS);
        struct rb_root* tree = malloc(sizeof(struct rb_root));
        init_rb_root(tree);
        srand(time(NULL));

        for (int i = 0; i < TEST_INSERT_KEYS; i++) {
                int v = rand();
                answer_struct_insert(answers, i, v);
                struct tree_node* tn = malloc(sizeof(struct tree_node));
                tn->key = i;
                tn->value = v;
                rb_insert(tree, &tn->node, tree_less);
        }
        mu_check(rb_is_rbtree(tree));

        for (int i = 0; i < TEST_INSERT_KEYS; i++) {
                int k = rand() % TEST_INSERT_KEYS;
                struct answer_node* answer = answer_struct_get(answers, k);
                mu_check(answer);
                struct rb_node* r = rb_search(tree, &k, tree_cmp_key);
                mu_check(r);
                struct tree_node* tn = rb_entry(r, struct tree_node, node);
                mu_check(tn->key == answer->key);
                mu_check(tn->value == answer->value);
        }

        for (int i = 0; i < TEST_ERASE_KEYS; i++) {
                int k = rand() % TEST_INSERT_KEYS;
                struct answer_node* answer = answer_struct_get(answers, k);
                if (!answer) {
                        continue;
                }
                struct rb_node* r = rb_search(tree, &k, tree_cmp_key);
                mu_check(r);
                rb_erase(tree, r);
                answer_struct_erase(answers, k);
        }
        mu_check(rb_is_rbtree(tree));

        struct answer_node* answer;
        for_each_in_list (
                answer, struct answer_node, node, &answers->answers_head) {
                struct rb_node* r = rb_search(tree, &answer->key, tree_cmp_key);
                mu_check(r);
                struct tree_node* tn = rb_entry(r, struct tree_node, node);
                mu_check(tn->key == answer->key);
                mu_check(tn->value == answer->value);
        }
}

MU_TEST_SUITE(fuzzing)
{
        MU_RUN_TEST(test_fuzzing_rbtree);
}

MU_TEST_SUITE(batch)
{
        MU_RUN_TEST(test_batch_processing);
}

int main()
{
        char* filename = "trace.txt";
        tracer = traced_rand_new(filename);
        MU_RUN_SUITE(fuzzing);
        MU_RUN_SUITE(batch);
        MU_REPORT();
        return minunit_status;
}
