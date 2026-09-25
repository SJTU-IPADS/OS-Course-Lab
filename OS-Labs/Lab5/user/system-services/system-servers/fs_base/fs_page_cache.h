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

#ifndef FS_PAGE_CACHE_H
#define FS_PAGE_CACHE_H

#include <sys/types.h>
#include <chcore/container/list.h>
#include <chcore/container/radix.h>
#include <pthread.h>

#define PAGE_CACHE_DEBUG 0

#if PAGE_CACHE_DEBUG == 1
#define page_cache_debug(fmt, ...)                                \
        printf(COLOR_YELLOW "<%s:%s:%d>: " COLOR_DEFAULT " " fmt, \
               __FILE__,                                          \
               __func__,                                          \
               __LINE__,                                          \
               ##__VA_ARGS__)
#else
#define page_cache_debug(fmt, ...) \
        do {                       \
        } while (0)
#endif

#define CACHED_PAGE_SIZE  4096
#define CACHED_BLOCK_SIZE 512
#define BLOCK_PER_PAGE    (CACHED_PAGE_SIZE / CACHED_BLOCK_SIZE)

#define ACTIVE_LIST_MAX   (1 << 14)
#define INACTIVE_LIST_MAX (1 << 14)
#define MAX_PINNED_PAGE   512
#define MAX_PAGE_CACHE_PAGE \
        (ACTIVE_LIST_MAX + INACTIVE_LIST_MAX + MAX_PINNED_PAGE)

#define WRITE_BACK_CYCLE 300

typedef off_t pidx_t;

/* List types. */
typedef enum {
        UNKNOWN_LIST = 0,
        ACTIVE_LIST,
        INACTIVE_LIST,
        PINNED_PAGES_LIST,
        INODE_PAGES_LIST,
} PAGE_CACHE_LIST_TYPE;

typedef enum {
        DIRECT = 0,
        WRITE_THROUGH,
        WRITE_BACK,
} PAGE_CACHE_STRATEGY;

typedef enum {
        READ = 0,
        WRITE,
} PAGE_CACHE_OPERATION_TYPE;

struct cached_page {
        /* Owner page_cache_entity_of_inode. */
        struct page_cache_entity_of_inode *owner;

        /* Which file page it caches. */
        pidx_t file_page_idx;

        /* Dirty flag for each block. */
        bool dirty[BLOCK_PER_PAGE];

        /* Cached contents. */
        char *content;

        /*
         * Indicates which list this page locates at.
         * A page should always be in two types of list.
         * One must be INODE_PAGES_LIST and the other can be
         * ACTIVE_LIST, INACTIVE_LIST or PINNED_PAGES_LIST.
         * The possible values of this field are:
         * UNKNOWN_LIST, ACTIVE_LIST, INACTIVE_LIST,
         * PINNED_PAGES_LIST, INODE_PAGES_LIST.
         * Note: INODE_PAGES_LIST means that this page is only in
         * INODE_PAGES_LIST. The other types mean that this page is
         * both in INODE_PAGES_LIST and this type.
         */
        PAGE_CACHE_LIST_TYPE in_which_list;

        /* Used for 2-list strategy. */
        struct list_head two_list_node;

        /* Used for pinned pages. */
        struct list_head pinned_pages_node;

        /* Used for page_cache_entity_of_inode. */
        struct list_head inode_pages_node;

        /* Page lock. */
        pthread_rwlock_t page_rwlock;
};

struct cached_pages_list {
        /* Used for easily traversing all pages. */
        struct list_head queue;

        /* List size. */
        int size;
};

struct page_cache_entity_of_inode {
        /* Owner inode index. */
        ino_t host_idx;

        /*
         * Used for easily traversing all pages and
         * quickly finding a specific page.
         */
        struct cached_pages_list pages;

        /* Used for quickly finding a specific page. */
        struct radix idx2page;

        /* Number of pages */
        int pages_cnt;

        /* Private data used for file read/write functions. */
        void *private_data;
};

/* Read a specific page from file. */
typedef int (*file_reader_t)(char *buf, pidx_t file_page_idx,
                             void *private_data);

/* Write a specific block or page(when page_block_idx == -1) to file. */
typedef int (*file_writer_t)(char *buf, pidx_t file_page_idx,
                             int page_block_idx, void *private_data);

typedef int (*event_handler_t)(void *private);

struct user_defined_funcs {
        /*
         * (Essential)
         * read/write behavior defined by module user
         */
        file_reader_t file_read;
        file_writer_t file_write;

        /*
         * (Optional, NULL if not used)
         * User may do something when pce turns empty to non-empty (combined
         * with a private)
         */
        event_handler_t handler_pce_turns_nonempty;
        event_handler_t handler_pce_turns_empty;
};

struct fs_page_cache {
        /*
         * Using two-list strategy to maintain caches.
         * Once a cold block is accessed, append it to second list. If a block
         * in second list is accessed, boost to first list. Both lists use LRU.
         */
        struct cached_pages_list active_list;
        struct cached_pages_list inactive_list;

        /* pinned pages list. */
        struct cached_pages_list pinned_pages_list;

        /* Each inode can use different cache strategy. */
        PAGE_CACHE_STRATEGY cache_strategy;

        /* functions supported by page cache user */
        struct user_defined_funcs user_func;

        pthread_mutex_t page_cache_lock;
};

/*
 * Initialize fs_page_cache.
 */
void fs_page_cache_init(PAGE_CACHE_STRATEGY strategy,
                        struct user_defined_funcs *user_func);

/*
 * Alloc a new page_cache_entity_of_inode.
 */
struct page_cache_entity_of_inode *
new_page_cache_entity_of_inode(ino_t host_idx, void *private_data);

/*
 * Switch page cache strategy.
 * Return: if succeed, return current strategy,
 *	 if failed, return -1.
 */
int page_cache_switch_strategy(PAGE_CACHE_STRATEGY new_strategy);

/*
 * Check if a specific page has been cached.
 * Return: if page exists, return 1,
 *	 if page doesn't exists, return 0.
 */
int page_cache_check_page(struct page_cache_entity_of_inode *pce,
                          pidx_t file_page_idx);

/*
 * Get a block or a page from corresponding page cache.
 * If page_block_idx == -1, read a page,
 * else read a block.
 * Return: if failed, return NULL.
 */
char *page_cache_get_block_or_page(struct page_cache_entity_of_inode *pce,
                                   pidx_t file_page_idx, int page_block_idx,
                                   PAGE_CACHE_OPERATION_TYPE op_type);

/*
 * Put a block or a page to corresponding page cache.
 */
void page_cache_put_block_or_page(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx, int page_block_idx,
                                  PAGE_CACHE_OPERATION_TYPE op_type);

/*
 * Flush a block or a page in corresponding page cache,
 * the cache still remains.
 * If page_block_idx == -1, flush a page,
 * else flush a block.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_flush_block_or_page(struct page_cache_entity_of_inode *pce,
                                   pidx_t file_page_idx, int page_block_idx);

/*
 * Flush all dirty pages that belongs to an inode,
 * the cache still remains.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_flush_pages_of_inode(struct page_cache_entity_of_inode *pce);

/*
 * Flush all dirty pages.
 * the cache still remains.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_flush_all_pages(void);

/*
 * Pin a single page.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_pin_single_page(struct page_cache_entity_of_inode *pce,
                               pidx_t file_page_idx);

/*
 * Unpin a single page.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_unpin_single_page(struct page_cache_entity_of_inode *pce,
                                 pidx_t file_page_idx);

/*
 * Pin multiple pages (@page_num pages from @file_page_idx).
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_pin_multiple_pages(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx, u64 page_num);

/*
 * Unpin multiple pages (@page_num pages from @file_page_idx).
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_unpin_multiple_pages(struct page_cache_entity_of_inode *pce,
                                    pidx_t file_page_idx, u64 page_num);

/*
 * Evict a single page.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_evict_single_page(struct page_cache_entity_of_inode *pce,
                                 pidx_t file_page_idx);

/*
 * Evict all pages of inode.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_evict_pages_of_inode(struct page_cache_entity_of_inode *pce);

/*
 * Delete a single page without flushing.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_delete_single_page(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx);

/*
 * Delete all pages of inode without flushing.
 * Return: if succeed, return 0,
 *	 if failed, return -1.
 */
int page_cache_delete_pages_of_inode(struct page_cache_entity_of_inode *pce);

#if PAGE_CACHE_DEBUG == 1
/* debug */
void print_all_list(void);
void test_fs_page_cache(void);
#endif

#endif /* FS_PAGE_CACHE_H */