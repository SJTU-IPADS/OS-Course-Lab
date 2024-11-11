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

#ifdef CHCORE
#include <common/kprint.h>
#include <common/macro.h>
#include <common/radix.h>
#endif
#include <mm/kmalloc.h>
#include <common/errno.h>

struct radix *new_radix(void)
{
        struct radix *radix;

        radix = kzalloc(sizeof(*radix));
        BUG_ON(!radix);

        return radix;
}

void init_radix(struct radix *radix)
{
        radix->root = kzalloc(sizeof(*radix->root));
        BUG_ON(!radix->root);
        radix->value_deleter = NULL;

        lock_init(&radix->radix_lock);
}

void init_radix_w_deleter(struct radix *radix, void (*value_deleter)(void *))
{
        init_radix(radix);
        radix->value_deleter = value_deleter;
}

static struct radix_node *new_radix_node(void)
{
        struct radix_node *n = kzalloc(sizeof(struct radix_node));

        if (!n) {
                kwarn("run-out-memoroy: cannot allocate radix_new_node whose size is %ld\n",
                      sizeof(struct radix_node));
                return ERR_PTR(-ENOMEM);
        }

        return n;
}

#ifndef FBINFER
int radix_add(struct radix *radix, u64 key, void *value)
{
        int ret;
        struct radix_node *node;
        struct radix_node *new;
        u16 index[RADIX_LEVELS];
        int i;
        int k;

        lock(&radix->radix_lock);
        if (!radix->root) {
                new = new_radix_node();
                if (IS_ERR(new)) {
                        ret = -ENOMEM;
                        goto fail_out;
                }
                radix->root = new;
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
                        new = new_radix_node();
                        if (IS_ERR(new)) {
                                ret = -ENOMEM;
                                goto fail_out;
                        }
                        node->children[k] = new;
                }
                node = node->children[k];
        }

        /* the leaf level */
        k = index[0];

        if ((node->values[k] != NULL) && (value != NULL)) {
                kwarn("Radix: add an existing key\n");
                BUG_ON(1);
        }

        node->values[k] = value;

        unlock(&radix->radix_lock);
        return 0;

fail_out:
        unlock(&radix->radix_lock);
        return ret;
}

void *radix_get(struct radix *radix, u64 key)
{
        void *ret;
        struct radix_node *node;
        u16 index[RADIX_LEVELS];
        int i;
        int k;

        lock(&radix->radix_lock);
        if (!radix->root) {
                ret = NULL;
                goto out;
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
                        ret = NULL;
                        goto out;
                }
                node = node->children[k];
        }

        /* the leaf level */
        k = index[0];
        ret = node->values[k];

out:
        unlock(&radix->radix_lock);
        return ret;
}

/* We use NULL to indicate deletion. */
int radix_del(struct radix *radix, u64 key)
{
        return radix_add(radix, key, NULL);
}

static void radix_free_node(struct radix_node *node, int node_level,
                            void (*value_deleter)(void *))
{
        int i;

        if (!node) {
                BUG("should not try to free a node pointed by NULL");
        }
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
        kfree(node);
}
#endif

int radix_free(struct radix *radix)
{
        lock(&radix->radix_lock);
        if (!radix || !radix->root) {
                WARN("trying to free an empty radix tree");
                return -EINVAL;
        }

        // recurssively free nodes and values (if value_deleter is not NULL)
        radix_free_node(radix->root, 0, radix->value_deleter);
        unlock(&radix->radix_lock);

        kfree(radix);
        return 0;
}
