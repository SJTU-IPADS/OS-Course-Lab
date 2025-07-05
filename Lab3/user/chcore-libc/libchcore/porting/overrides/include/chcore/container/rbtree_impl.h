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

#define RB_RED   (0)
#define RB_BLACK (1)

#define __rb_parent(pc)   ((struct rb_node *)(pc & ~3))
#define __rb_color(pc)    (pc & 1)
#define __rb_is_black(pc) (__rb_color(pc) == RB_BLACK)
#define __rb_is_red(pc)   (__rb_color(pc) == RB_RED)

#define rb_parent(node)     (__rb_parent(node->__parent_color))
#define rb_color(node)      (__rb_color(node->__parent_color))
#define rb_is_black(node)   (__rb_is_black(node->__parent_color))
#define rb_is_red(node)     (__rb_is_red(node->__parent_color))
#define rb_red_parent(node) ((struct rb_node *)node->__parent_color)
#define rb_has_red_child(node)                             \
        ((node->left_child && rb_is_red(node->left_child)) \
         || (node->right_child && rb_is_red(node->right_child)))

void __rb_insert_color(struct rb_root *root, struct rb_node *node);

/*
 * set parent and color helpers
 * Due to malloc/kmalloc 4-bytes alignment, and alignment attributes specified
 * in the defination of rb_node, we can use a compressed layout to store parent
 * rb_node pointer and color of a node in one field, saving 1 byte per rb_node.
 *
 * Thanks to alignment, at least the leftmost 3 bits of a rb_node pointer would
 * be 0. We leverage MLB of __parent_color to store color of a node. 0
 * represents a red node, and 1 represents a black one. This also indicates that
 * a new node is naturally red, which is desired in rbtree.
 */
static inline void rb_set_parent(struct rb_node *this, struct rb_node *parent)
{
        this->__parent_color = (unsigned long)parent | rb_color(this);
}

static inline void rb_set_color(struct rb_node *this, int color)
{
        this->__parent_color = (unsigned long)rb_parent(this) | (color);
}

static inline void rb_set_parent_color(struct rb_node *this,
                                       struct rb_node *parent, int color)
{
        this->__parent_color = (unsigned long)parent | (color & 3);
}

static inline void __rb_link_node(struct rb_node *node, struct rb_node *parent,
                                  struct rb_node **child_link)
{
        node->__parent_color = 0;
        rb_set_parent(node, parent);
        node->left_child = node->right_child = NULL;
        *child_link = node;
}
