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

#ifndef COMMON_RADIX_H
#define COMMON_RADIX_H

#include <common/types.h>
#include <common/lock.h>
#include <common/macro.h>

/* Each tree level represents RADIX_NODE_BITS bits of the key */
#define RADIX_NODE_BITS (4)
#define RADIX_NODE_SIZE (1 << (RADIX_NODE_BITS))
#define RADIX_NODE_MASK (RADIX_NODE_SIZE - 1)
#define RADIX_MAX_BITS (64)

#define RADIX_LEVELS (DIV_ROUND_UP(RADIX_MAX_BITS, RADIX_NODE_BITS))

struct radix_node {
	union {
		struct radix_node *children[RADIX_NODE_SIZE];
		void *values[RADIX_NODE_SIZE];
	};
};
struct radix {
	struct radix_node *root;
	struct lock radix_lock;
	void (*value_deleter)(void *);
};

/* interfaces */
struct radix *new_radix(void);
void init_radix(struct radix *radix);
int radix_add(struct radix *radix, u64 key, void *value);
void *radix_get(struct radix *radix, u64 key);
int radix_free(struct radix *radix);
int radix_del(struct radix *radix, u64 key);

void init_radix_w_deleter(struct radix *radix, void (*value_deleter)(void *));

#endif /* COMMON_RADIX_H */