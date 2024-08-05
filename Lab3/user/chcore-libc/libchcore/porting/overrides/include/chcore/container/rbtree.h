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
#include <chcore/type.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rb_node {
        unsigned long __parent_color;
        struct rb_node *right_child;
        struct rb_node *left_child;
} __attribute__((aligned(sizeof(long))));
#define rb_entry(node, type, field) container_of(node, type, field)

/*
 * wrapper type, distinguish between tree root and other nodes
 * in rbtree interfaces.
 * We have to differentiate rb_root from rb_node. For rb_node, users should
 * embed it into themselves types, and retrieve pointer of their types from
 * rb_node. However, users also need to hold a whole rbtree in some high-level
 * types, e.g., mm_struct in Linux owns a rbtree storing vmr nodes. In the
 * latter case, we should not let users embed a root rb_node in mm_struct,
 * otherwise, rb_entry to that node would return a pointer of mm_struct. But
 * users expect it to return a pointer of vmr. So we provide the rb_root type
 * representing the whole rbtree, and users should embed this type into those
 * high-level types like mm_struct.
 */
struct rb_root {
        struct rb_node *root_node;
};

static inline void init_rb_root(struct rb_root *root)
{
        root->root_node = NULL;
}

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
typedef int (*comp_node_func)(const struct rb_node *lhs,
                              const struct rb_node *rhs);
typedef int (*comp_key_func)(const void *key, const struct rb_node *node);
typedef bool (*less_func)(const struct rb_node *lhs, const struct rb_node *rhs);

/*
 * search interfaces
 */
/*
 * rb_search - search for a node matching @key in rbtree
 *
 * @this: rbtree to be searched
 * @key: pointer to user-specific key. We use void* to implement generality
 * @cmp: user-provided compare function, comparing @key and rb_node in rbtree.
 * Users should implement their logic to extract key from rb_node pointer.
 *
 * return:
 *     pointer of targeted node if @key exists in rbtree.
 *     NULL if no matching node is found.
 */
struct rb_node *rb_search(struct rb_root *this, const void *key,
                          comp_key_func cmp);
/*
 * rb_search - search for the **leftmost** node matching @key in rbtree
 *
 * @this: rbtree to be searched
 * @key: pointer to user-specific key. We use void* to implement generality
 * @cmp: user-provided compare function.
 *
 * return:
 *     pointer of targeted node if @key exists in rbtree.
 *     NULL if no matching node is found.
 */
struct rb_node *rb_search_first(struct rb_root *this, const void *key,
                                comp_key_func cmp);

/*
 * insert interfaces
 */
/*
 * rb_insert - insert a given @node into rbtree. @node
 * will be inserted as right child of existing nodes if they have the same key.
 *
 * @this: rbtree to be inserted
 * @node: node to be inserted. Users should embed rb_node in their custom types
 * , allocate those types and provide pointers of rb_node in those types.
 * @less: user-provided less function
 *
 */
void rb_insert(struct rb_root *this, struct rb_node *data, less_func less);

/*
 * erase interfaces
 */
/*
 * rb_erase - remove @node from rbtree @this. Due to rb_node embedded in users'
 * custom types, @node is not a pointer to head of a heap block. So rb_erase
 * won't free memory of @node. User code should free memory using their types
 * after erase rb_node contained in those types from @this.
 *
 * @this: rbtree to be erased
 * @node: node to be erased
 */
void rb_erase(struct rb_root *this, struct rb_node *node);

/*
 * replace interfaces
 */
/*
 * rb_replace_node - replace @old with @new directly without rebalancing and
 * recoloring. Users should make sure that keys of old and new are identical,
 * otherwise the rbtree would be inconsistent.
 *
 * @this: rbtree to be replaced
 * @old: old node in @this
 * @new: new node to take over @old, should has identical key with @old.
 */
void rb_replace_node(struct rb_root *this, struct rb_node *old,
                     struct rb_node *new);

/*
 * traverse interfaces
 */
/*
 * rb_next - return the next node of @node in rbtree. If @node is the last node,
 * return NULL.
 */
struct rb_node *rb_next(const struct rb_node *node);
/*
 * rb_prev - return the previous node of @node in rbtree. If @node is the first
 * node, return NULL.
 */
struct rb_node *rb_prev(const struct rb_node *node);
/*
 * rb_first - return the first node of rbtree. If rbtree is empty, return NULL.
 */
struct rb_node *rb_first(const struct rb_root *this);
/*
 * rb_last - return the last node of rbtree. If rbtree is empty, return NULL.
 */
struct rb_node *rb_last(const struct rb_root *this);
/*
 * rb_next_match - return the next node of @node if it matches @key at the same
 * time. Otherwise, return NULL. This function can be used to iterate over all
 * nodes with a given key in rbtree.
 */
struct rb_node *rb_next_match(const struct rb_node *node, const void *key,
                              comp_key_func cmp);

#define rb_for_each(root, node) \
        for (node = rb_first(root); node; node = rb_next(node))

#define rb_key_for_each(root, node, key, cmp)              \
        for (node = rb_search_first(root, key, cmp); node; \
             node = rb_next_match(node, key, cmp))

#ifdef __cplusplus
}
#endif