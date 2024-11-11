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
#include <string.h>
#include <sys/mman.h>
#include <chcore/idman.h>
#include <chcore/container/list.h>
#include <chcore/container/radix.h>
#include <chcore/bug.h>
#include <pthread.h>

#include "include/block_cache.h"

/*
 * @args:
 *   device_read usage: device_read(char *buffer, int lba)
 *   device_write usage: device_write(char *buffer, int lba)
 */
void bc_init(struct storage_block_cache *bc, device_reader_t device_read,
	     device_writer_t device_write)
{
	init_bc_list(&bc->first_list);
	init_bc_list(&bc->second_list);
	pthread_mutex_init(&bc->bc_mutex_lock, NULL);
	bc->device_read = device_read;
	bc->device_write = device_write;
}

static inline void bc_lock(struct storage_block_cache *bc)
{
	pthread_mutex_lock(&bc->bc_mutex_lock);
}

static inline void bc_unlock(struct storage_block_cache *bc)
{
	pthread_mutex_unlock(&bc->bc_mutex_lock);
}

static struct bc_node *__new_bc_node(addr_t addr, char *content)
{
	struct bc_node *r = malloc(sizeof(*r));
	BUG_ON(r == NULL);
	r->dirty = 0;
	r->addr = addr;
	r->content = (char *)malloc(BLOCK_SIZE);
	BUG_ON(r->content == NULL);

	if (content != NULL)
		memcpy(r->content, content, BLOCK_SIZE);

	return r;
}

static struct bc_node *__get_bc_node_from_list(struct bc_list *cache_list,
					       addr_t addr)
{
#ifdef BC_LIST_MAP_USING_RADIX
	struct bc_node *r = radix_get(&cache_list->addr2node, addr);
	return r;
#endif

#ifdef BC_LIST_MAP_USING_HASH
	struct bc_node *r;
	struct hlist_head *lst =
		htable_get_bucket(&cache_list->addr2node, addr);
	for_each_in_hlist (r, hnode, lst) {
		if (r->addr == addr)
			return r;
	}
	return NULL;
#endif
}

static struct bc_node *__top_node(struct bc_list *cache_list)
{
	if (list_empty(&cache_list->queue)) {
		return NULL;
	}
	return head2node((cache_list->queue).next);
}

/*
 * Write block in disk
 */
static inline void flush_node(struct storage_block_cache *bc, struct bc_node *n)
{
	int ret;

	if (n->dirty) {
		ret = bc->device_write(n->content, n->addr, bc->write_curry);
		BUG_ON(ret != BLOCK_SIZE);
	}

	n->dirty = 0;
}

/*
 * Append node at tail
 */
static inline void push_node(struct bc_list *cache_list, struct bc_node *n)
{
	list_append(&n->node, &cache_list->queue);

#ifdef BC_LIST_MAP_USING_RADIX
	radix_add(&cache_list->addr2node, n->addr, n);
#endif

#ifdef BC_LIST_MAP_USING_HASH
	htable_add(&cache_list->addr2node, n->addr, &n->hnode);
#endif

	cache_list->size++;
}

/*
 * Remove node at head, free corresponding content
 */
static inline void pop_node(struct storage_block_cache *bc,
			    struct bc_list *cache_list)
{
	struct bc_node *n;
	n = __top_node(cache_list);
	flush_node(bc, n);
	list_del(&n->node);

#ifdef BC_LIST_MAP_USING_RADIX
	radix_del(&cache_list->addr2node, n->addr, 0);
#endif

#ifdef BC_LIST_MAP_USING_HASH
	htable_del(&n->hnode);
#endif

	cache_list->size--;
	if (n->content) {
		free(n->content);
	}
	free(n);
}

/*
 * Boost a node in first list. Remove the node and append it at tail.
 */
static inline void boost_node1(struct storage_block_cache *bc,
			       struct bc_node *n)
{
	bc_dbg("addr:%d\n", n->addr);

	list_del(&n->node);
	list_append(&n->node, &bc->first_list.queue);
}

/*
 * Boost a node in second list. Remove the node and append it in first list
 */
static inline void boost_node2(struct storage_block_cache *bc,
			       struct bc_node *n)
{
	bc_dbg("addr:%d\n", n->addr);

	/* Delete node from second list */
	list_del(&n->node);

#ifdef BC_LIST_MAP_USING_RADIX
	radix_del(&bc->second_list.addr2node, n->addr, 0);
#endif

#ifdef BC_LIST_MAP_USING_HASH
	htable_del(&n->hnode);
#endif

	bc->second_list.size--;

	/* Add node from first list */
	push_node(&bc->first_list, n);
}

/*
 * Read block[lba], store in buffer_out
 */
void bc_read(struct storage_block_cache *bc, int lba, char *buffer_out)
{
	bc_dbg("[bc_read] lba=%d\n", lba);
	int ret;

#ifdef BC_DIRECT
	ret = bc->device_read(buffer_out, lba);
	if (ret != BLOCK_SIZE) {
		printf("[bc_read] Error occurs when invoke device_read\n");
	}
#else
	bc_lock(bc);

	struct bc_node *n;

	n = __get_bc_node_from_list(&bc->first_list, lba);
	if (n) {
		/* hit in first list */
		memcpy(buffer_out, n->content, BLOCK_SIZE);
		boost_node1(bc, n);
		goto out;
	}

	n = __get_bc_node_from_list(&bc->second_list, lba);
	if (n) {
		/* hit in second list */
		memcpy(buffer_out, n->content, BLOCK_SIZE);
		boost_node2(bc, n);
		if (bc->first_list.size > FIRST_LIST_MAX) {
			pop_node(bc, &bc->first_list);
		}
		goto out;
	}

	/* construct new_node without memory copying (NULL) */
	n = __new_bc_node(lba, NULL);

	/* block not in cache, fetch from disk */
	ret = bc->device_read(n->content, lba, bc->read_curry);
	BUG_ON(ret != BLOCK_SIZE);

	push_node(&bc->second_list, n);
	if (bc->second_list.size > SECOND_LIST_MAX) {
		pop_node(bc, &bc->second_list);
	}

	memcpy(buffer_out, n->content, BLOCK_SIZE);

out:
	bc_unlock(bc);
	return;
#endif
}

static inline void mark_cache_node(struct storage_block_cache *bc,
				   struct bc_node *n)
{
	n->dirty = 1;
#ifdef BC_WRITE_THROUGH
	flush_node(bc, n);
#endif
}

void bc_write(struct storage_block_cache *bc, char *buf, int lba)
{
	bc_dbg("[bc_write] lba=%d\n", lba);

#ifdef BC_DIRECT
	int ret;
	ret = bc->device_write(buf, lba);
	if (ret != BLOCK_SIZE) {
		printf("[bc_write] Error occurs when invoke device_write\n");
	}
#else
	bc_lock(bc);

	struct bc_node *n;

	n = __get_bc_node_from_list(&bc->first_list, lba);
	if (n) {
		/* hit in first list */
		memcpy(n->content, buf, BLOCK_SIZE);
		mark_cache_node(bc, n);
		boost_node1(bc, n);
		goto out;
	}

	n = __get_bc_node_from_list(&bc->second_list, lba);
	if (n) {
		/* hit in second list */
		memcpy(n->content, buf, BLOCK_SIZE);
		mark_cache_node(bc, n);
		boost_node2(bc, n);
		if (bc->second_list.size > SECOND_LIST_MAX) {
			pop_node(bc, &bc->second_list);
		}
		goto out;
	}

	/* block not in cache */
	n = __new_bc_node(lba, buf);
	mark_cache_node(bc, n);
	push_node(&bc->second_list, n);
	if (bc->second_list.size > SECOND_LIST_MAX) {
		pop_node(bc, &bc->second_list);
	}

out:
	bc_unlock(bc);
	return;
#endif
}

/************************************************************************/

/* for debug */
#ifdef BC_LIST_MAP_USING_RADIX
void print_two_list(struct storage_block_cache *bc)
{
	struct bc_node *n;
	int i, j;

	printf("[block_cache INFO] first cache contains:\n");
	printf("list info:\n");
	for_each_in_list (n, struct bc_node, node, &bc->first_list.queue) {
		printf("%d  ", n->addr);
	}
	printf("\n");
	printf("radix info:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 10; j++) {
			printf("%3d", 10 * i + j);
		}
		printf("\n");
		for (j = 0; j < 10; j++) {
			if (radix_get(&bc->first_list.addr2node, 10 * i + j)) {
				printf("Y  ");
			}
		}
		printf("\n");
	}

	printf("[block_cache INFO] second cache contains:\n");
	for_each_in_list (n, struct bc_node, node, &bc->second_list.queue) {
		printf("%d  ", n->addr);
	}
	printf("\n");
	printf("radix info:\n");
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 10; j++) {
			printf("%3d", 10 * i + j);
		}
		printf("\n");
		for (j = 0; j < 10; j++) {
			if (radix_get(&bc->second_list.addr2node, 10 * i + j)) {
				printf("Y  ");
			}
		}
		printf("\n");
	}

	printf("[block_cache INFO] end\n");
}

#define ADDR(i) (i % 2 ? 16 + i : 16 - i)

void bc_list_test(struct storage_block_cache *bc)
{
	struct bc_node *n;

	int i;
	for (i = 0; i < 15; i++) {
		n = __new_bc_node(ADDR(i), NULL);
		push_node(&bc->first_list, n);
	}

	print_two_list(bc);

	n = __get_bc_node_from_list(&bc->first_list, ADDR(2));
	printf("ADDR(2)->addr = %d\n", n->addr);
	n = __top_node(&bc->first_list);
	printf("TOP->addr = %d\n", n->addr);

	pop_node(bc, &bc->first_list);
	print_two_list(bc);
}

#endif
