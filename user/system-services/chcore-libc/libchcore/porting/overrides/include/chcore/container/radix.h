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

#include <chcore/bug.h>
#include <chcore/error.h>
#include <chcore/type.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: bug exists on hikey if we set RADIX_NODE_BITS to 4 */
#define RADIX_NODE_BITS (9)
#define RADIX_NODE_SIZE (1 << (RADIX_NODE_BITS))
#define RADIX_NODE_MASK (RADIX_NODE_SIZE - 1)
#define RADIX_MAX_BITS  (64)

/* ceil(a/b) */
#define DIV_UP(a, b) (((a) + (b)-1) / (b))

#define RADIX_LEVELS (DIV_UP(RADIX_MAX_BITS, RADIX_NODE_BITS))

struct radix_node {
        union {
                struct radix_node *children[RADIX_NODE_SIZE];
                void *values[RADIX_NODE_SIZE];
        };
};
struct radix {
        struct radix_node *root;
        void (*value_deleter)(void *);
};

static inline void init_radix(struct radix *radix)
{
        /* TODO: use the real calloc */
        /* radix->root = calloc(1, sizeof(*radix->root)); */
        radix->root = (struct radix_node *)calloc(1, sizeof(*radix->root));
        BUG_ON(!radix->root);
        radix->value_deleter = NULL;
}

static inline void init_radix_w_deleter(struct radix *radix,
                                        void (*value_deleter)(void *))
{
        init_radix(radix);
        radix->value_deleter = value_deleter;
}

static inline struct radix_node *new_radix_node(void)
{
        /* TODO: use the real calloc */
        /* struct radix_node *n = calloc(1, sizeof(struct radix_node)); */
        struct radix_node *n =
                (struct radix_node *)calloc(1, sizeof(struct radix_node));

        if (!n)
                return (struct radix_node *)CHCORE_ERR_PTR(-ENOMEM);

        return n;
}

static inline int radix_add(struct radix *radix, u64 key, void *value)
{
        struct radix_node *node;
        struct radix_node *new_node;
        u16 index[RADIX_LEVELS];
        int i;
        int k;

        if (!radix->root) {
                new_node = new_radix_node();
                if (CHCORE_IS_ERR(new_node))
                        return -ENOMEM;
                radix->root = new_node;
        }
        node = radix->root;

        /* calculate index for each level */
        for (i = 0; i < RADIX_LEVELS; ++i) {
                index[i] = key & RADIX_NODE_MASK;
                key >>= RADIX_NODE_BITS;
        }

        /* the intermediate levels */
        for (i = RADIX_LEVELS - 1; i > 0; --i) {
                k = index[i];
                if (!node->children[k]) {
                        new_node = new_radix_node();
                        if (CHCORE_IS_ERR(new_node))
                                return -ENOMEM;
                        node->children[k] = new_node;
                }
                node = node->children[k];
        }

        /* the leaf level */
        k = index[0];
        node->values[k] = value;

        return 0;
}

static inline void *radix_get(struct radix *radix, u64 key)
{
        struct radix_node *node;
        u16 index[RADIX_LEVELS];
        int i;
        int k;

        if (!radix->root)
                return NULL;
        node = radix->root;

        /* calculate index for each level */
        for (i = 0; i < RADIX_LEVELS; ++i) {
                index[i] = key & RADIX_NODE_MASK;
                key >>= RADIX_NODE_BITS;
        }

        /* the intermediate levels */
        for (i = RADIX_LEVELS - 1; i > 0; --i) {
                k = index[i];
                if (!node->children[k])
                        return NULL;
                node = node->children[k];
        }

        /* the leaf level */
        k = index[0];
        return node->values[k];
}

/* FIXME(MK): We should allow users to store NULL in radix... */
__attribute__((__unused__)) static inline int
radix_del(struct radix *radix, u64 key, int delete_value)
{
        struct radix_node *node;
        u16 index[RADIX_LEVELS];
        int i;
        int k;

        if (!radix->root)
                return -1;
        node = radix->root;

        /* calculate index for each level */
        for (i = 0; i < RADIX_LEVELS; ++i) {
                index[i] = key & RADIX_NODE_MASK;
                key >>= RADIX_NODE_BITS;
        }

        /* the intermediate levels */
        for (i = RADIX_LEVELS - 1; i > 0; --i) {
                k = index[i];
                if (!node->children[k])
                        return -1;
                node = node->children[k];
        }

        /* the leaf level */
        k = index[0];
        if (radix->value_deleter && delete_value)
                radix->value_deleter(node->values[k]);
        node->values[k] = NULL;
        return 0;
}

static inline void radix_free_node(struct radix_node *node, int node_level,
                                   void (*value_deleter)(void *))
{
        int i;

        WARN_ON(!node, "should not try to free a node pointed by NULL");

        if (node_level == RADIX_LEVELS - 1) {
                if (value_deleter) {
                        for (i = 0; i < RADIX_NODE_SIZE; i++) {
                                if (node->values[i])
                                        value_deleter(node->values[i]);
                        }
                }
        } else {
                for (i = 0; i < RADIX_NODE_SIZE; i++) {
                        if (node->children[i])
                                radix_free_node(node->children[i],
                                                node_level + 1,
                                                value_deleter);
                }
        }
        free(node);
}

static inline int radix_free(struct radix *radix)
{
        if (!radix || !radix->root) {
                WARN("trying to free an empty radix tree");
                return -EINVAL;
        }

        // recurssively free nodes and values (if value_deleter is not NULL)
        radix_free_node(radix->root, 0, radix->value_deleter);

        return 0;
}

typedef int (*radix_scan_cb)(void *value, void *privdata);

static inline int __radix_scan(struct radix_node *node, int node_level,
                               u64 start, radix_scan_cb cb, void *data)
{
        int start_i;
        int i;
        int err;
        u64 mask;
        int shift;

        WARN_ON(!node, "should not try to free a node pointed by NULL");

        shift = (RADIX_LEVELS - node_level - 1) * RADIX_NODE_BITS;
        mask = RADIX_NODE_MASK << shift;
        start_i = (start & mask) >> shift;

        if (node_level == RADIX_LEVELS - 1) {
                for (i = start_i; i < RADIX_NODE_SIZE; i++) {
                        if (!node->values[i])
                                continue;
                        err = cb(node->values[i], data);
                        if (err)
                                return err;
                }
                return 0;
        }

        for (i = start_i; i < RADIX_NODE_SIZE; i++) {
                if (node->children[i]) {
                        err = __radix_scan(node->children[i],
                                           node_level + 1,
                                           start,
                                           cb,
                                           data);
                        if (err)
                                return err;
                }
                start = 0;
        }

        return 0;
}

/**
 * Scan the radix from @start (inclusive), to the end.
 */
static inline int radix_scan(struct radix *radix, u64 start, radix_scan_cb cb,
                             void *cb_args)
{
        return __radix_scan(radix->root, 0, start, cb, cb_args);
}

#ifdef __cplusplus
}
#endif
