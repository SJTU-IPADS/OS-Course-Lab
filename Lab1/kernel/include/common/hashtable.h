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

#ifndef COMMON_HASHTABLE_H
#define COMMON_HASHTABLE_H

#include <common/types.h>
#include <common/list.h>
#include <mm/kmalloc.h>

struct htable {
	struct hlist_head *buckets;
	int size;
};


static inline void init_htable(struct htable *ht, int size)
{
	ht->size = size;
	ht->buckets = kzalloc(sizeof(*ht->buckets) * size);
}

static inline void htable_add(struct htable *ht, u32 key,
			      struct hlist_node *node)
{
	hlist_add(node, &ht->buckets[key % ht->size]);
}

static inline void htable_del(struct hlist_node *node)
{
	hlist_del(node);
}

static inline struct hlist_head *htable_get_bucket(struct htable *ht, u32 key)
{
	return &ht->buckets[key % ht->size];
}

static inline bool htable_empty(struct htable *ht)
{
	int i;

	for (i = 0; i < ht->size; ++i)
		if (!hlist_empty(&ht->buckets[i]))
			return false;
	return true;
}

static inline int htable_free(struct htable *ht)
{
	if (!ht || !ht->buckets) {
		WARN("trying to free an empty htable");
		return -EINVAL;
	}

	// we don't free individual buckets (i.e., the hlist) since their nodes
	// a not allocated by htable_xxx or hlist_xxx
	
	kfree(ht->buckets);

	return 0;
}


#define for_each_in_htable(elem, b, field, ht) \
	for ((b) = 0, (elem) = NULL; (elem) == NULL && (b) < (ht)->size; ++(b)) \
		for_each_in_hlist(elem, field, &(ht)->buckets[b])

#define for_each_in_htable_safe(elem, tmp, b, field, ht) \
	for ((b) = 0, (elem) = NULL; (elem) == NULL && (b) < (ht)->size; ++(b)) \
		for_each_in_hlist_safe(elem, tmp, field, &(ht)->buckets[b])

#endif /* COMMON_HASHTABLE_H */