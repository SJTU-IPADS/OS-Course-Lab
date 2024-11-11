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

#include <pthread.h>

#include <chcore/container/list.h>
#include <chcore/container/radix.h>
#include <chcore/container/hashtable.h>

#ifdef __cplusplus
extern "C" {
#endif

#define addr_t         int
#define head2node(ptr) container_of(ptr, struct bc_node, node)

/*
 * FIXME: storage device may have various BLOCK_SIZE (not only 512)
 */
#define BLOCK_SIZE      512
#define FIRST_LIST_MAX  32
#define SECOND_LIST_MAX 32

/*
 * 0: direct r/w disk
 * 1: write-back
 * 2: write-through
 */
#define CACHE_STRATEGY 2

#if CACHE_STRATEGY == 0
#define BC_DIRECT
#elif CACHE_STRATEGY == 1
#define BC_WRITE_BACK
#elif CACHE_STRATEGY == 2
#define BC_WRITE_THROUGH
#endif

/*
 * Configure type for `addr2node` mapping
 */
#if 0
#define BC_LIST_MAP_USING_RADIX
#else
#define BC_LIST_MAP_USING_HASH
#define BC_LIST_HASH_SIZE 32
#endif

#if 0
#define bc_dbg(fmt, ...) \
	printf("<%s:%d>: " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define bc_dbg(fmt, ...) \
	do {             \
	} while (0)
#endif

struct bc_node {
	int dirty;
	addr_t addr;
	char *content;
	struct list_head node;

#ifdef BC_LIST_MAP_USING_HASH
	struct hlist_node hnode;
#endif
};

struct bc_list {
	struct list_head queue;

#ifdef BC_LIST_MAP_USING_RADIX
	struct radix addr2node;
#endif

#ifdef BC_LIST_MAP_USING_HASH
	struct htable addr2node;
#endif

	int size;
};

typedef int (*device_reader_t)(char *, int, void *);
typedef int (*device_writer_t)(char *, int, void *);

struct storage_block_cache {
	/*
         * Using two-list strategy to maintain caches.
         * Once a cold block is accessed, append it to second list. If a block in
         *   second list is accessed, boost to first list. Both lists use LRU.
         */
	struct bc_list first_list;
	struct bc_list second_list;

	/* Using `bc_mutex_lock` to execute `do_read` and `do_write` alternately */
	pthread_mutex_t bc_mutex_lock;

	/* Function pointers to read/write to real device */
	device_reader_t device_read;
	device_writer_t device_write;

	/* For curry arguments, the third `device_*` should always be `*_curry` */
	void *read_curry;
	void *write_curry;
};

static inline void init_bc_list(struct bc_list *list)
{
	init_list_head(&list->queue);

#ifdef BC_LIST_MAP_USING_RADIX
	init_radix(&list->addr2node);
#endif

#ifdef BC_LIST_MAP_USING_HASH
	init_htable(&list->addr2node, BC_LIST_HASH_SIZE);
#endif

	list->size = 0;
}

void bc_init(struct storage_block_cache *bc, device_reader_t device_read,
	     device_writer_t device_write);
void bc_read(struct storage_block_cache *bc, int lba, char *buffer_out);
void bc_write(struct storage_block_cache *bc, char *buf, int lba);
void bc_list_test(struct storage_block_cache *bc);

#ifdef __cplusplus
}
#endif
