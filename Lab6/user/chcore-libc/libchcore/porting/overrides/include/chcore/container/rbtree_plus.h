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

#pragma once

#include <chcore/container/rbtree.h>
#include <chcore/container/list.h>

/**
 * @brief A node in a rbtree plus. Users can embed this struct like rb_node
 * or list_head in their own types to achieve generic in C.
 *
 * In implementation, we exploit a "two-level" generic to re-use existing
 * rbtree and list implementation while providing a simple interface to users.
 * The first level generic is for users: they embed rbp_node in their types,
 * pass rbp_node* to our APIs, and use rbp_entry to get their types back.
 *
 * The second level is internally used by us. We break a rbp_node into a
 * rb_node and a list_head, then link them into a rbtree and a list
 * respectively. Decided by the APIs invoked by users, we levergae rb_entry or
 * list_entry to get rbp_node* back, and return them to users.
 */
struct rbp_node {
        struct rb_node rbnode;
        struct list_head list_node;
};

/**
 * @brief RB+ Tree, or rbtree plus, is a red-black tree variant which support
 * fast traversal operation of its nodes in order. This name is inspired by
 * B+ Tree, which has a similar traverse feature.
 *
 * It's implemented by combining a rbtree and a list, and hooking insert/erase
 * operation to manipulate rbtree and list simultaneously. Although it's a
 * combined data structure, users can still use it without any knowledge of the
 * underlying data structures, they just need to embed a rbp_node in their own
 * types, simliar to usages of rbtree and list.
 */
struct rbtree_plus {
        struct rb_root root;
        struct list_head head;
};

void init_rbtree_plus(struct rbtree_plus *this);

#define rbp_entry(ptr, type, member) container_of(ptr, type, member)

/*
 * compare functions defination
 * We expect those functions have following semantics:
 * cmp_xxx_func:
 *     return 0 if keys of left operand equal to right operand
 *     return <0 if left < right
 *     return >0 if left > right
 * less_func:
 *     return true if left < right
 *     return false if left >= right
 */
typedef int (*comp_rbp_key_func)(const void *key, const struct rbp_node *node);
typedef bool (*less_rbp_func)(const struct rbp_node *lhs,
                              const struct rbp_node *rhs);

/*
 * search interfaces
 */
/*
 * rbp_search - search for a node matching @key in rbtree plus
 *
 * @this: rbtree plus to be searched
 * @key: pointer to user-specific key. We use void* to implement generality
 * @cmp: user-provided compare function, comparing @key and rbp_node in rbtree
 * plus. Users should implement their logic to extract key from rbp_node
 * pointer.
 *
 * return:
 *     pointer of targeted node if @key exists in rbtree plus.
 *     NULL if no matching node is found.
 */
struct rbp_node *rbp_search(struct rbtree_plus *this, const void *key,
                            comp_rbp_key_func cmp);
/*
 * rbp_search - search for the **leftmost** node matching @key in rbtree plus
 *
 * @this: rbtree plus to be searched
 * @key: pointer to user-specific key. We use void* to implement generality
 * @cmp: user-provided compare function.
 *
 * return:
 *     pointer of targeted node if @key exists in rbtree plus.
 *     NULL if no matching node is found.
 */
struct rbp_node *rbp_search_first(struct rbtree_plus *this, const void *key,
                                  comp_rbp_key_func cmp);

/*
 * rbp_search - search for the node **nearest** to @key in rbtree plus
 *
 * If there is a node matches @key, it would be returned. Otherwhise,
 * this method would return the node whose key is the greatest in nodes
 * that are less than @key. Or the node whose key is the least in nodes
 * that are greater than @key. In other words, it would return the node
 * that serve as "insertion point" for @key.
 *
 * @this: rbtree plus to be searched
 * @key: pointer to user-specific key. We use void* to implement generality
 * @cmp: user-provided compare function.
 *
 * return:
 *     pointer of targeted node if @key exists in rbtree plus.
 *     NULL if no matching node is found.
 */
struct rbp_node *rbp_search_nearest(struct rbtree_plus *this, const void *key,
                                    comp_rbp_key_func cmp);

/*
 * insert interfaces
 */
/*
 * rbp_insert - insert a given @node into rbtree plus. @node
 * will be inserted as right child of existing nodes if they have the same key.
 *
 * @this: rbtree plus to be inserted
 * @node: node to be inserted. Users should embed rbp_node in their custom types
 * , allocate those types and provide pointers of rbp_node in those types.
 * @less: user-provided less function
 *
 */
void rbp_insert(struct rbtree_plus *this, struct rbp_node *data,
                less_rbp_func less);

/*
 * erase interfaces
 */
/*
 * rbp_erase - remove @node from rbtree plus @this. Due to rbp_node embedded in
 * users' custom types, @node is not a pointer to head of a heap block. So
 * rbp_erase won't free memory of @node. User code should free memory using
 * their types after erase rbp_node contained in those types from @this.
 *
 * @this: rbtree plus to be erased
 * @node: node to be erased
 */
void rbp_erase(struct rbtree_plus *this, struct rbp_node *node);

/*
 * replace interfaces
 */
/*
 * rbp_replace_node - replace @old with @new directly without rebalancing and
 * recoloring. Users should make sure that keys of old and new are identical,
 * otherwise the rbtree plus would be inconsistent.
 *
 * @this: rbtree plus to be replaced
 * @old: old node in @this
 * @new: new node to take over @old, should has identical key with @old.
 */
void rbp_replace_node(struct rbtree_plus *this, struct rbp_node *old,
                      struct rbp_node *new);

/*
 * traverse interfaces
 */
/*
 * rbp_next - return the next node of @node in rbtree plus. If @node is the last
 * node, return NULL. To check whether @node is the last node, rbtree_plus it
 * belongs to is required: @this.
 */
struct rbp_node *rbp_next(const struct rbtree_plus *this,
                          const struct rbp_node *node);
/*
 * rbp_prev - return the previous node of @node in rbtree plus. If @node is the
 * first node, return NULL. To check whether @node is the first node,
 * rbtree_plus it belongs to is required: @this.
 */
struct rbp_node *rbp_prev(const struct rbtree_plus *this,
                          const struct rbp_node *node);
/*
 * rbp_first - return the first node of rbtree plus. If rbtree plus is empty,
 * return NULL.
 */
struct rbp_node *rbp_first(const struct rbtree_plus *this);
/*
 * rbp_last - return the last node of rbtree plus. If rbtree plus is empty,
 * return NULL.
 */
struct rbp_node *rbp_last(const struct rbtree_plus *this);
/*
 * rbp_next_match - return the next node of @node if it matches @key at the same
 * time. Otherwise, return NULL. This function can be used to iterate over all
 * nodes with a given key in rbtree plus.
 */
struct rbp_node *rbp_next_match(const struct rbtree_plus *this,
                                const struct rbp_node *node, const void *key,
                                comp_rbp_key_func cmp);

#define rbp_for_each(root, node) \
        for (node = rbp_first(root); node; node = rbp_next(root, node))

#define rbp_key_for_each(root, node, key, cmp)              \
        for (node = rbp_search_first(root, key, cmp); node; \
             node = rbp_next_match(root, node, key, cmp))

#define rbp_for_each_safe(root, node, tmp)                             \
        for (node = rbp_first(root), tmp = rbp_next(root, node); node; \
             node = tmp, tmp = rbp_next(root, node))
