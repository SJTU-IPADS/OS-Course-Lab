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

#include <stdbool.h>
#include <chcore/container/rbtree_plus.h>
#include <chcore/container/rbtree_impl.h>

struct comp_rb_key_wrapper_struct {
        const void *user_key;
        comp_rbp_key_func user_comp;
};

/**
 * @brief Wrapper around user provided @key and compare function of type
 * `comp_rbp_key_func`. Users just need to implement their compare function
 * comparing their custom key and a rbp_node. But internally, we leverage
 * existing rbtree search function to search a rbp_node containing @key given by
 * user, which requires a compare function of type `comp_rb_key_func`. So we
 * implement this wrapper to convert the user provided compare function to the
 * required `comp_rb_key_func` using data structure comp_rb_key_wrapper_struct.
 *
 * @param key A pointer to a comp_rb_key_wrapper_struct struct, which contains
 * user provided real key and compare function.
 * @param rb_node
 * @return int
 */
static int comp_rb_key_wrapper(const void *key, const struct rb_node *rb_node)
{
        struct comp_rb_key_wrapper_struct *wrapper =
                (struct comp_rb_key_wrapper_struct *)key;
        struct rbp_node *rbp_node = rb_entry(rb_node, struct rbp_node, rbnode);

        return wrapper->user_comp(wrapper->user_key, rbp_node);
}

void init_rbtree_plus(struct rbtree_plus *this)
{
        init_rb_root(&this->root);
        init_list_head(&this->head);
}

struct rbp_node *rbp_search(struct rbtree_plus *this, const void *key,
                            comp_rbp_key_func cmp)
{
        struct rb_node *ret;
        /**
         * Construct a wrapper struct with user provided key and compare
         * function. See comp_rbp_key_func for more details.
         */
        struct comp_rb_key_wrapper_struct wrapper = {
                .user_key = key,
                .user_comp = cmp,
        };

        ret = rb_search(&this->root, &wrapper, comp_rb_key_wrapper);

        /**
         * !! Attention: if ret is NULL, it means the key is not found in the
         * tree. We should return NULL too instead of invoke rb_entry on ret,
         * which would generate a non-NULL but invalid (negative in fact)
         * pointer.
         */
        if (ret == NULL) {
                return NULL;
        }

        return rb_entry(ret, struct rbp_node, rbnode);
}

struct rbp_node *rbp_search_first(struct rbtree_plus *this, const void *key,
                                  comp_rbp_key_func cmp)
{
        struct rb_node *ret;
        struct comp_rb_key_wrapper_struct wrapper = {
                .user_key = key,
                .user_comp = cmp,
        };

        ret = rb_search_first(&this->root, &wrapper, comp_rb_key_wrapper);

        /**
         * !! Attention: if ret is NULL, it means the key is not found in the
         * tree. We should return NULL too instead of invoke rb_entry on ret,
         * which would generate a non-NULL but invalid (negative in fact)
         * pointer.
         */
        if (ret == NULL) {
                return NULL;
        }

        return rb_entry(ret, struct rbp_node, rbnode);
}

struct rbp_node *rbp_search_nearest(struct rbtree_plus *this, const void *key,
                                    comp_rbp_key_func cmp)
{
        struct rbp_node *cur_rbp_node, *ret = NULL;
        struct rb_node *cur = this->root.root_node;
        int cmp_ret;
        while (cur) {
                cur_rbp_node = rb_entry(cur, struct rbp_node, rbnode);
                cmp_ret = cmp(key, cur_rbp_node);
                ret = cur_rbp_node;
                if (cmp_ret < 0) {
                        cur = cur->left_child;
                } else if (cmp_ret > 0) {
                        cur = cur->right_child;
                } else {
                        break;
                }
        }

        return ret;
}

void rbp_insert(struct rbtree_plus *this, struct rbp_node *data,
                less_rbp_func less)
{
        struct rb_node **new_link = &this->root.root_node;
        struct rb_node *parent = NULL, *cur = this->root.root_node;
        bool insert_at_left = false;
        struct rbp_node *parent_rbp_node;

        while (cur) {
                if (less(data, rb_entry(cur, struct rbp_node, rbnode))) {
                        parent = cur;
                        new_link = &cur->left_child;
                        cur = cur->left_child;
                        insert_at_left = true;
                } else {
                        parent = cur;
                        new_link = &cur->right_child;
                        cur = cur->right_child;
                        insert_at_left = false;
                }
        }

        /**
         * After the binary search logic above, we have the following
         * invariants:
         *
         * @parent points to parent node of the new rbp_node(if the tree is not
         * empty).
         * @insert_at_left indicates whether the new rbp_node should be inserted
         * as the left child of @parent or right child of @parent.
         */
        if (list_empty(&this->head)) {
                /**
                 * If the tree is empty, the new rbp_node is the root node of
                 * rbtree plus. We just insert it as the first node of the list.
                 */
                list_add(&data->list_node, &this->head);
        } else {
                /**
                 * If the tree is not empty, we need to insert the new rbp_node
                 * into an appropriate position in the list. Choosing the
                 * position is rely on
                 * @insert_at_left. If @insert_at_left is true, the key in @data
                 * is smaller than @parent and greater than the previous node of
                 * @parent in the list. So we just insert @data after
                 * @parent.prev. So as if @insert_at_left is false.
                 *
                 * Because there is a special list_head(this->head) in the list,
                 * the logic above is always correct even though there is only 1
                 * node in the rbtree plus.
                 */
                parent_rbp_node = rb_entry(parent, struct rbp_node, rbnode);
                if (insert_at_left) {
                        list_add(&data->list_node,
                                 parent_rbp_node->list_node.prev);
                } else {
                        list_add(&data->list_node, &parent_rbp_node->list_node);
                }
        }

        __rb_link_node(&data->rbnode, parent, new_link);
        __rb_insert_color(&this->root, &data->rbnode);
}

void rbp_erase(struct rbtree_plus *this, struct rbp_node *node)
{
        list_del(&node->list_node);
        rb_erase(&this->root, &node->rbnode);
}

void rbp_replace_node(struct rbtree_plus *this, struct rbp_node *old,
                      struct rbp_node *new)
{
        struct list_head *old_prev = old->list_node.prev;
        rb_replace_node(&this->root, &old->rbnode, &new->rbnode);
        list_del(&old->list_node);
        list_add(&new->list_node, old_prev);
}

struct rbp_node *rbp_next(const struct rbtree_plus *this,
                          const struct rbp_node *node)
{
        if (!node) {
                return NULL;
        }
        if (node->list_node.next == &this->head) {
                return NULL;
        }
        return list_entry(node->list_node.next, struct rbp_node, list_node);
}

struct rbp_node *rbp_prev(const struct rbtree_plus *this,
                          const struct rbp_node *node)
{
        if (!node) {
                return NULL;
        }
        if (node->list_node.prev == &this->head) {
                return NULL;
        }
        return list_entry(node->list_node.prev, struct rbp_node, list_node);
}

struct rbp_node *rbp_first(const struct rbtree_plus *this)
{
        if (list_empty(&this->head)) {
                return NULL;
        }
        return list_entry(this->head.next, struct rbp_node, list_node);
}

struct rbp_node *rbp_last(const struct rbtree_plus *this)
{
        if (list_empty(&this->head)) {
                return NULL;
        }
        return list_entry(this->head.prev, struct rbp_node, list_node);
}

struct rbp_node *rbp_next_match(const struct rbtree_plus *this,
                                const struct rbp_node *node, const void *key,
                                comp_rbp_key_func cmp)
{
        struct rbp_node *ret = rbp_next(this, node);
        if (ret && cmp(key, ret) != 0) {
                ret = NULL;
        }
        return ret;
}