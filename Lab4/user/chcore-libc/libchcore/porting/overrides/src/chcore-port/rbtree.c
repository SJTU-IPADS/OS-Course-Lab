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
#include <chcore/container/rbtree_impl.h>

static inline void __rb_change_child(struct rb_root *root,
                                     struct rb_node *parent,
                                     struct rb_node *old, struct rb_node *new)
{
        if (parent) {
                if (parent->left_child == old) {
                        parent->left_child = new;
                } else {
                        parent->right_child = new;
                }
        } else {
                root->root_node = new;
        }
        if (new) {
                rb_set_parent(new, parent);
        }
}

/*
 * Reference: 邓俊辉. 数据结构: C++ 语言版. 清华大学出版社
 * All double rotation variants (zip-zap, zap-zip, zip-zip, zap-zap...)
 * can be unified to following connect34 operations. Because those
 * operations are aimed at rebalancing three unbalanced nodes to the
 * standard balanced state, like this:
 *          b
 *        /  \
 *        a   c
 *      / \   / \
 *     t1 t2 t3  t4
 * There are three nodes and 4 corresponding subtrees, that's why this
 * function is named as connect34.
 * Differente kinds of double rotations only need to set arguments of
 * this function according to their targeted tree patterns. See codes
 * in __rb_insert_color and __rb_erase_color for concrete examples.
 */
static inline void __connect34(struct rb_node *a, struct rb_node *b,
                               struct rb_node *c, struct rb_node *t1,
                               struct rb_node *t2, struct rb_node *t3,
                               struct rb_node *t4)
{
        a->left_child = t1;
        if (t1) {
                rb_set_parent(t1, a);
        }
        a->right_child = t2;
        if (t2) {
                rb_set_parent(t2, a);
        }
        c->left_child = t3;
        if (t3) {
                rb_set_parent(t3, c);
        }
        c->right_child = t4;
        if (t4) {
                rb_set_parent(t4, c);
        }
        b->left_child = a;
        b->right_child = c;
        rb_set_parent(a, b);
        rb_set_parent(c, b);
}

/*
 * We use characters to represent a tree node. Lower case characters indicates
 * a red node, and upper case ones indicate a black node. Characters enclosed in
 * brackets represent any color. We assumed that every node has two subtrees,
 * they can be actually subtree with identical blackheight, or NULL representing
 * external nodes.
 */
void __rb_insert_color(struct rb_root *root, struct rb_node *node)
{
        struct rb_node *parent = rb_parent(node), *gparent = NULL,
                       *uncle = NULL;

        while (true) {
                /*
                 * Loop invariant
                 * - @parent and @node is red
                 */
                if (!parent) {
                        rb_set_color(node, RB_BLACK);
                        break;
                }

                if (rb_is_black(parent)) {
                        break;
                }

                // if double-red exists, then gparent must not be NULL.
                gparent = rb_red_parent(parent);

                if (parent == gparent->left_child) {
                        uncle = gparent->right_child;
                        if (!uncle || rb_is_black(uncle)) {
                                // uncle is black or NULL.
                                if (node == parent->left_child) {
                                        /*
                                         * Case-1-A
                                         * Matched tree pattern:
                                         *       G                P
                                         *      / \              / \
                                         *     p    U    ->     x   g
                                         *    /                      \
                                         *   x                        U
                                         */

                                        // __connect34 only reconnect node,
                                        // parent and gparent, but we still have
                                        // to connect new subtree root back to
                                        // original tree, i.e, let parent
                                        // inherit gparent's parent.
                                        __rb_change_child(root,
                                                          rb_parent(gparent),
                                                          gparent,
                                                          parent);

                                        __connect34(node,
                                                    parent,
                                                    gparent,
                                                    node->left_child,
                                                    node->right_child,
                                                    parent->right_child,
                                                    uncle);
                                        rb_set_color(parent, RB_BLACK);
                                        rb_set_color(gparent, RB_RED);

                                        // gparent might be root node, but
                                        // rotation here would alter root node
                                        // update root node pointer stored in
                                        // root
                                        if (!rb_parent(parent)) {
                                                root->root_node = parent;
                                        }
                                } else {
                                        /*
                                         * Case-1-B
                                         * Matched tree pattern:
                                         *       G                X
                                         *      / \              / \
                                         *     p    U    ->     p   g
                                         *      \                    \
                                         *       x                    U
                                         */
                                        __rb_change_child(root,
                                                          rb_parent(gparent),
                                                          gparent,
                                                          node);

                                        __connect34(parent,
                                                    node,
                                                    gparent,
                                                    parent->left_child,
                                                    node->left_child,
                                                    node->right_child,
                                                    uncle);
                                        rb_set_color(node, RB_BLACK);
                                        rb_set_color(gparent, RB_RED);

                                        if (!rb_parent(node)) {
                                                root->root_node = node;
                                        }
                                }
                                break;
                        } else {
                                // uncle must be red.

                                /*
                                 * Case-2
                                 * Matched tree pattern:
                                 *       G                g
                                 *      / \              / \
                                 *     p    u    ->     P   U
                                 *    /                /
                                 *   x                x
                                 * g might be new source of double-red. So we
                                 * set @node to g, and @parent to parent of g
                                 * and continue iteration.
                                 */

                                rb_set_color(gparent, RB_RED);
                                rb_set_color(parent, RB_BLACK);
                                rb_set_color(uncle, RB_BLACK);
                                node = gparent;
                                parent = rb_parent(gparent);
                                continue;
                        }
                } else {
                        // mirror
                        uncle = gparent->left_child;
                        if (!uncle || rb_is_black(uncle)) {
                                if (node == parent->right_child) {
                                        // mirror of Case-1-A
                                        __rb_change_child(root,
                                                          rb_parent(gparent),
                                                          gparent,
                                                          parent);
                                        __connect34(gparent,
                                                    parent,
                                                    node,
                                                    uncle,
                                                    parent->left_child,
                                                    node->left_child,
                                                    node->right_child);
                                        rb_set_color(parent, RB_BLACK);
                                        rb_set_color(gparent, RB_RED);
                                        if (!rb_parent(parent)) {
                                                root->root_node = parent;
                                        }
                                        break;
                                } else {
                                        // mirror of Case-1-B
                                        __rb_change_child(root,
                                                          rb_parent(gparent),
                                                          gparent,
                                                          node);
                                        __connect34(gparent,
                                                    node,
                                                    parent,
                                                    uncle,
                                                    node->left_child,
                                                    node->right_child,
                                                    parent->right_child);
                                        rb_set_color(node, RB_BLACK);
                                        rb_set_color(gparent, RB_RED);
                                        if (!rb_parent(node)) {
                                                root->root_node = node;
                                        }
                                        break;
                                }
                        } else {
                                // mirror of Case-2
                                rb_set_color(gparent, RB_RED);
                                rb_set_color(parent, RB_BLACK);
                                rb_set_color(uncle, RB_BLACK);
                                node = gparent;
                                parent = rb_parent(gparent);
                                continue;
                        }
                }
        }
}

void rb_insert(struct rb_root *this, struct rb_node *data, less_func less)
{
        struct rb_node **new_link = &this->root_node;
        struct rb_node *parent = NULL, *cur = this->root_node;

        while (cur) {
                if (less(data, cur)) {
                        parent = cur;
                        new_link = &cur->left_child;
                        cur = cur->left_child;
                } else {
                        parent = cur;
                        new_link = &cur->right_child;
                        cur = cur->right_child;
                }
        }

        __rb_link_node(data, parent, new_link);
        __rb_insert_color(this, data);
}

void rb_replace_node(struct rb_root *this, struct rb_node *old,
                     struct rb_node *new)
{
        *new = *old;

        if (new->left_child) {
                rb_set_parent(new->left_child, new);
        }

        if (new->right_child) {
                rb_set_parent(new->right_child, new);
        }

        __rb_change_child(this, rb_parent(old), old, new);
}

static struct rb_node *__rb_erase(struct rb_root *this, struct rb_node *node,
                                  bool *is_left_child)
{
        struct rb_node *succ, *parent = rb_parent(node), *ret = NULL,
                              *succ_child;

        if (!node->left_child) {
                // right_child might be NULL, if so and node is black, then
                // double-black exists.
                if (!node->right_child) {
                        // erase the only root_node requires no additional
                        // processing
                        if (rb_is_black(node) && parent) {
                                ret = parent;
                                *is_left_child = node == parent->left_child;
                        }
                } else {
                        // If node is black and has only the right_child, the
                        // latter must be red and the former must be black, so
                        // we just set the right_child as black.
                        rb_set_color(node->right_child, RB_BLACK);
                }
                // Calling rb_replace_node here is not appropriate, because node
                // might has no child.
                __rb_change_child(this, parent, node, node->right_child);
        } else if (!node->right_child) {
                // node has at least one child(left child). If right_child is
                // NULL, it's impossible for left_child to be black, otherwise
                // the rbtree is illegal. So checking double-black or recoloring
                // the left child are unnecessary here.
                __rb_change_child(this, parent, node, node->left_child);
                rb_set_color(node->left_child, RB_BLACK);
        } else {
                // find succ of node first.
                succ = node->right_child;
                if (!succ->left_child) {
                        succ_child = succ->right_child;
                        // check whether double-black exists. Logically, we
                        // should exchange node and succ first, then check
                        // against node and succ_child. But it's redundant,
                        // because node will be erased finally. So we check
                        // double-black using info of succ directly.
                        if (rb_is_black(succ)) {
                                if (succ_child && rb_is_red(succ_child)) {
                                        rb_set_color(succ_child, RB_BLACK);
                                } else {
                                        // due to succ is node->right_child,
                                        // after adjustment, parent of
                                        // succ_child is still succ.
                                        ret = succ;
                                        // succ_child will be located at
                                        // original pos of succ, and succ must
                                        // be right child of node here.
                                        *is_left_child = false;
                                }
                        }
                        // succ of node is its right child. We just need to
                        // let succ inherit parent, color and **left child** of
                        // node.
                        __rb_change_child(this, parent, node, succ);
                        rb_set_color(succ, rb_color(node));
                        succ->left_child = node->left_child;
                        rb_set_parent(node->left_child, succ);
                } else {
                        while (succ->left_child) {
                                succ = succ->left_child;
                        }
                        parent = rb_parent(succ);

                        succ_child = succ->right_child;

                        // check whether double-black exists
                        if (rb_is_black(succ)) {
                                if (succ_child && rb_is_red(succ_child)) {
                                        rb_set_color(succ_child, RB_BLACK);
                                } else {
                                        ret = parent;
                                        *is_left_child = succ
                                                         == parent->left_child;
                                }
                        }

                        // replace succ with its right_child
                        __rb_change_child(this, parent, succ, succ_child);

                        // replace node with succ, succ would inherit node's
                        // parent, color and **chilren**.
                        rb_replace_node(this, node, succ);
                }
        }

        return ret;
}

static void __rb_erase_color(struct rb_root *root, struct rb_node *parent,
                             bool is_left_child)
{
        struct rb_node *node = is_left_child ? parent->left_child :
                                               parent->right_child;
        struct rb_node *slibing, *nephew;

        while (true) {
                /*
                 * Loop invariants
                 * - @parent is not NULL(@node is not root node)
                 * - subtree of which @parent as root has 1 lower blackheight
                 */
                if (!parent) {
                        break;
                }
                if (node == parent->left_child) {
                        // slibing must not be NULL.
                        slibing = parent->right_child;
                        if (rb_is_black(slibing)) {
                                if (slibing->right_child
                                    && rb_is_red(slibing->right_child)) {
                                        nephew = slibing->right_child;
                                        /*
                                         * Case-1-A (x: @node, n: @nephew)
                                         * Matched tree pattern:
                                         *      (p)              (s)
                                         *      / \              / \
                                         *     X   S    ->      P   N
                                         *        / \          / \
                                         *      (s1) n        X  (s1)
                                         */

                                        __rb_change_child(root,
                                                          rb_parent(parent),
                                                          parent,
                                                          slibing);

                                        __connect34(parent,
                                                    slibing,
                                                    nephew,
                                                    node,
                                                    slibing->left_child,
                                                    nephew->left_child,
                                                    nephew->right_child);
                                        rb_set_color(slibing, rb_color(parent));
                                        rb_set_color(parent, RB_BLACK);
                                        rb_set_color(nephew, RB_BLACK);

                                        if (!rb_parent(slibing)) {
                                                root->root_node = slibing;
                                        }
                                        break;
                                } else if (slibing->left_child
                                           && rb_is_red(slibing->left_child)) {
                                        nephew = slibing->left_child;
                                        /*
                                         * Case-1-B (x: @node, n: @nephew)
                                         * Matched tree pattern:
                                         *      (p)              (n)
                                         *      / \              / \
                                         *     X   S    ->      P   S
                                         *        / \          /     \
                                         *       n  (s1)      X      (s1)
                                         */
                                        __rb_change_child(root,
                                                          rb_parent(parent),
                                                          parent,
                                                          nephew);

                                        __connect34(parent,
                                                    nephew,
                                                    slibing,
                                                    node,
                                                    nephew->left_child,
                                                    nephew->right_child,
                                                    slibing->right_child);
                                        rb_set_color(nephew, rb_color(parent));
                                        rb_set_color(parent, RB_BLACK);

                                        if (!rb_parent(nephew)) {
                                                root->root_node = nephew;
                                        }
                                        break;
                                } else {
                                        if (rb_is_black(parent)) {
                                                /*
                                                 * Case-2 (x: @node, n: @nephew)
                                                 * Matched tree pattern:
                                                 *       P                P
                                                 *      / \              / \
                                                 *     X   S    ->      X   s
                                                 *        / \               / \
                                                 *       S1  S2            S1 S2
                                                 * Subtree P has 1 lower
                                                 * blackheight than slibings of
                                                 * P. (X has 1 lower blackheight
                                                 * than S before). So we let
                                                 * @node = P and continue
                                                 * iteration.
                                                 */
                                                rb_set_color(slibing, RB_RED);
                                                node = parent;
                                                parent = rb_parent(node);
                                                continue;
                                        } else {
                                                /*
                                                 * Case-3 (x: @node, n: @nephew)
                                                 * Matched tree pattern:
                                                 *       p                P
                                                 *      / \              / \
                                                 *     X   S    ->      X   s
                                                 *        / \               / \
                                                 *       S1  S2            S1 S2
                                                 */
                                                rb_set_color(parent, RB_BLACK);
                                                rb_set_color(slibing, RB_RED);
                                                break;
                                        }
                                }
                        } else {
                                /*
                                 * Case-4 (x: @node, n: @nephew)
                                 * Matched tree pattern:
                                 *       P                S
                                 *      / \              / \
                                 *     X   s    ->      p   S2
                                 *        / \          / \
                                 *       S1  S2       X   S1
                                 * We do not fix rbtree here. However, after
                                 * this rotation, X has a black slibing now, and
                                 * will turn into Case-1 or Case-3 (depends on
                                 * children of S1).
                                 */
                                __rb_change_child(root,
                                                  rb_parent(parent),
                                                  parent,
                                                  slibing);

                                parent->right_child = slibing->left_child;
                                if (slibing->left_child) {
                                        rb_set_parent(slibing->left_child,
                                                      parent);
                                }

                                slibing->left_child = parent;
                                rb_set_parent(parent, slibing);

                                rb_set_color(slibing, RB_BLACK);
                                rb_set_color(parent, RB_RED);

                                if (!rb_parent(slibing)) {
                                        root->root_node = slibing;
                                }
                                continue;
                        }
                } else {
                        slibing = parent->left_child;
                        if (rb_is_black(slibing)) {
                                if (slibing->left_child
                                    && rb_is_red(slibing->left_child)) {
                                        // mirror of Case-1-A
                                        nephew = slibing->left_child;

                                        __rb_change_child(root,
                                                          rb_parent(parent),
                                                          parent,
                                                          slibing);
                                        __connect34(nephew,
                                                    slibing,
                                                    parent,
                                                    nephew->left_child,
                                                    nephew->right_child,
                                                    slibing->right_child,
                                                    node);
                                        rb_set_color(slibing, rb_color(parent));
                                        rb_set_color(nephew, RB_BLACK);
                                        rb_set_color(parent, RB_BLACK);

                                        if (!rb_parent(slibing)) {
                                                root->root_node = slibing;
                                        }
                                        break;
                                } else if (slibing->right_child
                                           && rb_is_red(slibing->right_child)) {
                                        // mirror of Case-1-B
                                        nephew = slibing->right_child;

                                        __rb_change_child(root,
                                                          rb_parent(parent),
                                                          parent,
                                                          nephew);

                                        __connect34(slibing,
                                                    nephew,
                                                    parent,
                                                    slibing->left_child,
                                                    nephew->left_child,
                                                    nephew->right_child,
                                                    node);
                                        rb_set_color(nephew, rb_color(parent));
                                        rb_set_color(parent, RB_BLACK);

                                        if (!rb_parent(nephew)) {
                                                root->root_node = nephew;
                                        }
                                        break;
                                } else {
                                        if (rb_is_black(parent)) {
                                                // mirror of Case-2
                                                rb_set_color(slibing, RB_RED);
                                                node = parent;
                                                parent = rb_parent(node);
                                                continue;
                                        } else {
                                                // mirror of Case-3
                                                rb_set_color(parent, RB_BLACK);
                                                rb_set_color(slibing, RB_RED);
                                                break;
                                        }
                                }
                        } else {
                                // mirror of Case-4
                                __rb_change_child(root,
                                                  rb_parent(parent),
                                                  parent,
                                                  slibing);
                                parent->left_child = slibing->right_child;
                                if (slibing->right_child) {
                                        rb_set_parent(slibing->right_child,
                                                      parent);
                                }

                                slibing->right_child = parent;
                                rb_set_parent(parent, slibing);

                                rb_set_color(slibing, RB_BLACK);
                                rb_set_color(parent, RB_RED);
                                continue;
                        }
                }
        }
}

void rb_erase(struct rb_root *this, struct rb_node *node)
{
        bool is_left_child;
        struct rb_node *rebalance = __rb_erase(this, node, &is_left_child);
        if (rebalance) {
                __rb_erase_color(this, rebalance, is_left_child);
        }
}

struct rb_node *rb_search(struct rb_root *this, const void *key,
                          comp_key_func cmp)
{
        struct rb_node *cur = this->root_node;
        int cmp_ret;
        while (cur) {
                cmp_ret = cmp(key, cur);
                if (cmp_ret < 0) {
                        cur = cur->left_child;
                } else if (cmp_ret > 0) {
                        cur = cur->right_child;
                } else {
                        return cur;
                }
        }

        return NULL;
}

struct rb_node *rb_search_first(struct rb_root *this, const void *key,
                                comp_key_func cmp)
{
        struct rb_node *cur = this->root_node, *ret = NULL;
        int c;
        while (cur) {
                c = cmp(key, cur);
                if (c <= 0) {
                        if (c == 0) {
                                ret = cur;
                        }
                        if (c != 0 && ret) {
                                break;
                        }
                        cur = cur->left_child;
                } else {
                        cur = cur->right_child;
                }
        }

        return ret;
}

struct rb_node *rb_next(const struct rb_node *node)
{
        struct rb_node *ret = NULL, *parent = NULL;

        // If node has right_child, find its direct successor
        if (node->right_child) {
                ret = node->right_child;
                while (ret->left_child) {
                        ret = ret->left_child;
                }
                return ret;
        }

        // If node has no right_child, backtrace to its parent.
        // If node is left_child of its parent, then parent is our next node.
        // If node is right_child of its parent, it indicates that parent is <=
        // node, so we continue backtracing, until find a node which is
        // left_child of its parent, or reach parent of root_node, i.e., NULL.
        while ((parent = rb_parent(node)) && node == parent->right_child)
                node = parent;

        return parent;
}

struct rb_node *rb_prev(const struct rb_node *node)
{
        // mirror of rb_next
        struct rb_node *ret = NULL, *parent = NULL;

        if (node->left_child) {
                ret = node->left_child;
                while (ret->right_child) {
                        ret = ret->right_child;
                }
                return ret;
        }

        while ((parent = rb_parent(node)) && node == parent->left_child)
                node = parent;

        return parent;
}

struct rb_node *rb_first(const struct rb_root *this)
{
        struct rb_node *cur = this->root_node;
        if (!cur) {
                return NULL;
        }

        while (cur->left_child) {
                cur = cur->left_child;
        }
        return cur;
}

struct rb_node *rb_last(const struct rb_root *this)
{
        struct rb_node *cur = this->root_node;
        if (!cur) {
                return NULL;
        }

        while (cur->right_child) {
                cur = cur->right_child;
        }
        return cur;
}

struct rb_node *rb_next_match(const struct rb_node *node, const void *key,
                              comp_key_func cmp)
{
        struct rb_node *ret = rb_next(node);
        if (ret && cmp(key, ret) != 0) {
                ret = NULL;
        }
        return ret;
}
