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

#include <malloc.h>
#include <pthread.h>
#include <stdlib.h>
#include <chcore/defs.h>
#include <chcore/memory.h>
#include <chcore/syscall.h>
#include <chcore-internal/fs_debug.h>

#include "fs_page_cache.h"
#include "fs_wrapper_defs.h"

static struct fs_page_cache page_cache = {0};

/* +++++++++++++++++++++++++++ Helper functions +++++++++++++++++++++++++++++ */

/*
 * if page_block_idx == -1, check a page's dirty flag,
 * else check a block's dirty flag.
 */
static bool is_block_or_page_dirty(struct cached_page *p, int page_block_idx)
{
        int i;

        if (page_block_idx != -1)
                return p->dirty[page_block_idx];
        else
                for (i = 0; i < BLOCK_PER_PAGE; ++i)
                        if (p->dirty[i])
                                return true;
        return false;
}

/*
 * if page_block_idx == -1, set a page's dirty flag as @is_dirty,
 * else set a block's dirty flag as @is_dirtyy.
 */
static void set_block_or_page_dirty(struct cached_page *p, int page_block_idx,
                                    bool is_dirty)
{
        int i;

        if (page_block_idx != -1)
                p->dirty[page_block_idx] = is_dirty;
        else
                for (i = 0; i < BLOCK_PER_PAGE; ++i)
                        p->dirty[i] = is_dirty;
}

/* ++++++++++++++++++++++++++ Page list operations ++++++++++++++++++++++++++ */
static inline void cached_pages_list_init(struct cached_pages_list *list)
{
        init_list_head(&list->queue);
        list->size = 0;
}

/* Return the head of the list. */
static struct cached_page *
cached_pages_list_top_node(struct cached_pages_list *list,
                           PAGE_CACHE_LIST_TYPE list_type)
{
        if (list_empty(&list->queue))
                return NULL;
        if (list_type == ACTIVE_LIST || list_type == INACTIVE_LIST)
                return list_entry(
                        list->queue.next, struct cached_page, two_list_node);
        else if (list_type == INODE_PAGES_LIST)
                return list_entry(
                        list->queue.next, struct cached_page, inode_pages_node);
        else if (list_type == PINNED_PAGES_LIST)
                return list_entry(list->queue.next,
                                  struct cached_page,
                                  pinned_pages_node);

        BUG("Unknown list type.");
        return NULL;
}

/* Find corresponding page from the page_cache_entity_of_inode. */
static inline struct cached_page *
get_node_from_page_cache_entity(struct page_cache_entity_of_inode *pce,
                                pidx_t file_page_idx)
{
        return radix_get(&pce->idx2page, file_page_idx);
}

/*
 * Append node to the tail of the list.
 */
static void cached_pages_list_append_node(struct cached_pages_list *list,
                                          struct cached_page *p,
                                          PAGE_CACHE_LIST_TYPE list_type)
{
        switch (list_type) {
        case ACTIVE_LIST:
        case INACTIVE_LIST:
                list_append(&p->two_list_node, &list->queue);
                break;
        case INODE_PAGES_LIST:
                list_append(&p->inode_pages_node, &list->queue);
                break;
        case PINNED_PAGES_LIST:
                list_append(&p->pinned_pages_node, &list->queue);
                break;
        default:
                BUG("Unknown list type.");
                break;
        }

        p->in_which_list = list_type;
        list->size++;
}

/*
 * Remove head node, don't forget to free the data outside this function.
 */
static void cached_pages_list_delete_node(struct cached_pages_list *list,
                                          struct cached_page *p,
                                          PAGE_CACHE_LIST_TYPE list_type)
{
        switch (list_type) {
        case ACTIVE_LIST:
        case INACTIVE_LIST:
                list_del(&p->two_list_node);
                p->in_which_list = INODE_PAGES_LIST;
                break;
        case INODE_PAGES_LIST:
                list_del(&p->inode_pages_node);
                p->in_which_list = UNKNOWN_LIST;
                break;
        case PINNED_PAGES_LIST:
                list_del(&p->pinned_pages_node);
                p->in_which_list = INODE_PAGES_LIST;
                break;
        default:
                BUG("Unknown list type.");
                break;
        }

        list->size--;
        BUG_ON(list->size < 0);
}

/* +++++++++++++++++++++++++ cached_page operations +++++++++++++++++++++++++ */
/* The caller should guarantee page_block_idx is in legal range. */
static int flush_single_block(struct cached_page *p, int page_block_idx)
{
        int ret = 0;

        BUG_ON(page_block_idx < 0);

        if (is_block_or_page_dirty(p, page_block_idx)) {
                page_cache_debug(
                        "[write_back_routine] %d:%d:%d in active list is dirty, write it back.\n",
                        p->owner->host_idx,
                        p->file_page_idx,
                        page_block_idx);
                if (page_cache.user_func.file_write(
                            p->content + page_block_idx * CACHED_BLOCK_SIZE,
                            p->file_page_idx,
                            page_block_idx,
                            p->owner->private_data)
                    <= 0) {
                        BUG("[write_back_all_pages] file_write failed\n");
                        ret = -1;
                        goto out;
                }
                set_block_or_page_dirty(p, page_block_idx, false);
        }

out:
        return ret;
}

static int flush_single_page(struct cached_page *p)
{
        int i, ret = 0;

        BUG_ON(p == NULL);

        for (i = 0; i < BLOCK_PER_PAGE; ++i) {
                if (flush_single_block(p, i) != 0) {
                        ret = -1;
                        goto out;
                }
        }
out:
        return ret;
}

static void free_page(struct cached_page *p)
{
        BUG_ON(p == NULL);

        free(p->content);

        /* Delete from ACTIVE_LIST, INACTIVE_LIST or PINNED_PAGES_LIST. */
        switch (p->in_which_list) {
        case ACTIVE_LIST:
                cached_pages_list_delete_node(
                        &page_cache.active_list, p, ACTIVE_LIST);
                break;
        case INACTIVE_LIST:
                cached_pages_list_delete_node(
                        &page_cache.inactive_list, p, INACTIVE_LIST);
                break;
        case PINNED_PAGES_LIST:
                cached_pages_list_delete_node(
                        &page_cache.pinned_pages_list, p, PINNED_PAGES_LIST);
                break;
        default:
                BUG("Try to free a page that is not in ACTIVE_LIST, INACTIVE_LIST or PINNED_PAGES_LIST.\n");
        }

        /* Delete from INODE_PAGES_LIST. */
        cached_pages_list_delete_node(&p->owner->pages, p, INODE_PAGES_LIST);
        radix_del(&p->owner->idx2page, p->file_page_idx, 0);

        /* Once pages_cnt in a pce reached 0, trigger handler */
        if (--(p->owner->pages_cnt) == 0
            && page_cache.user_func.handler_pce_turns_empty) {
                page_cache.user_func.handler_pce_turns_empty(
                        p->owner->private_data);
        }

        pthread_rwlock_destroy(&p->page_rwlock);

        free(p);
}

static void flush_and_free_page(struct cached_page *p)
{
        BUG_ON(p == NULL);

        /* Can't free a page that's being used. */
        pthread_rwlock_wrlock(&p->page_rwlock);

        /*
         * pages with empty contents is marked not dirty.
         * So it won't be flushed.
         */
        if (is_block_or_page_dirty(p, -1))
                flush_single_page(p);

        free_page(p);
}

static struct cached_page *new_page(struct page_cache_entity_of_inode *owner,
                                    pidx_t file_page_idx)
{
        struct cached_page *p;

        p = (struct cached_page *)calloc(1, sizeof(struct cached_page));
        if (!p)
                return NULL;

        p->owner = owner;
        p->file_page_idx = file_page_idx;

        init_list_head(&p->two_list_node);
        init_list_head(&p->inode_pages_node);
        init_list_head(&p->pinned_pages_node);

        pthread_rwlock_init(&p->page_rwlock, NULL);

        p->content = aligned_alloc(CACHED_PAGE_SIZE, CACHED_PAGE_SIZE);
        BUG_ON(!p->content);
        memset(p->content, 0, CACHED_PAGE_SIZE);

        /* Append to INODE_PAGES_LIST. */
        cached_pages_list_append_node(&owner->pages, p, INODE_PAGES_LIST);
        radix_add(&owner->idx2page, file_page_idx, p);

        /* Contribute 1 ref to pce for each cached_page */
        if (owner->pages_cnt++ == 0
            && page_cache.user_func.handler_pce_turns_nonempty) {
                page_cache.user_func.handler_pce_turns_nonempty(
                        owner->private_data);
        }

        return p;
}

static inline void evict_head_page(struct cached_pages_list *list,
                                   PAGE_CACHE_LIST_TYPE list_type)
{
        struct cached_page *p;

        /* Evict head page. */
        if (list_type == ACTIVE_LIST || list_type == INACTIVE_LIST) {
                p = cached_pages_list_top_node(list, list_type);
                page_cache_debug("[evict_head_page] Evict %d:%d in list %d.\n",
                                 p->owner->host_idx,
                                 p->file_page_idx,
                                 list_type);
                flush_and_free_page(p);
        } else {
                BUG("Try to evict a page that is not in two list.\n");
        }
}

/*
 * Boost a node in active list. Remove the node and append it at tail.
 */
static inline void boost_active_page(struct cached_page *p)
{
        /* Delete node from active list. */
        cached_pages_list_delete_node(&page_cache.active_list, p, ACTIVE_LIST);

        /* Append it to the tail. */
        cached_pages_list_append_node(&page_cache.active_list, p, ACTIVE_LIST);
}

/*
 * Boost a node in inactive list.
 * Remove the node and append it to the tail of the active list.
 */
static inline void boost_inactive_page(struct cached_page *p)
{
        struct cached_page *head_page;
        /* Delete node from inactive list */
        cached_pages_list_delete_node(
                &page_cache.inactive_list, p, INACTIVE_LIST);

        /* Add node to active list */
        cached_pages_list_append_node(&page_cache.active_list, p, ACTIVE_LIST);

        if (page_cache.active_list.size > ACTIVE_LIST_MAX) {
                /*
                 * If active list is full,
                 * move head unpinned page to inactive list.
                 */
                head_page = cached_pages_list_top_node(&page_cache.active_list,
                                                       ACTIVE_LIST);
                /**
                 * It should be impossible for head_page == NULL but klocwork
                 * reported a bug here, maybe due to precision of static
                 * analysis
                 */
                if (head_page == NULL) {
                        WARN("head_page == NULL");
                        return;
                }
                cached_pages_list_delete_node(
                        &page_cache.active_list, head_page, ACTIVE_LIST);
                cached_pages_list_append_node(
                        &page_cache.inactive_list, head_page, INACTIVE_LIST);

                if (page_cache.inactive_list.size > INACTIVE_LIST_MAX) {
                        /*
                         * if inactive list is full,
                         * evict head unpinned page.
                         */
                        evict_head_page(&page_cache.inactive_list,
                                        INACTIVE_LIST);
                }
        }
}

/* Caller should hold per page_cache mutex lock to protect list operations */
static inline void boost_page(struct cached_page *p)
{
        if (p->in_which_list == ACTIVE_LIST)
                boost_active_page(p);
        else if (p->in_which_list == INACTIVE_LIST)
                boost_inactive_page(p);
        else
                BUG("Try to boost page that is not in two lists!\n");
}

/*
 * Find a page from active_list, inactive_list and pinned_pages_list, if page
 * not found, create a new one and insert it to inactive list and pages list.
 * Notice: The caller should hold per page_cache mutex lock!
 * return value:
 *      @which_list: ACTIVE_LIST or INACTIVE_LIST or PINNED_PAGES_LIST.
 *      @is_new: if page is newly allocated, is_new = true, else is_new = false.
 */
struct cached_page *find_or_new_page(struct page_cache_entity_of_inode *pce,
                                     pidx_t file_page_idx, bool *is_new)
{
        struct cached_page *p;
        bool temp_is_new;

        p = get_node_from_page_cache_entity(pce, file_page_idx);
        if (p == NULL) {
#ifdef TEST_COUNT_PAGE_CACHE
                count.miss = count.miss + 1;
#endif
                /*
                 * Cache miss, create a new page
                 * and fill it with corresponding contents.
                 */
                page_cache_debug(
                        "[find_or_new_page] page cache %d:%d not found， alloc a new one\n",
                        pce->host_idx,
                        file_page_idx);
                p = new_page(pce, file_page_idx);
                if (p == NULL) {
                        BUG("[find_or_new_page] new_page failed\n");
                        goto fail;
                }

                /*
                 * According to two list strategy, a new page should
                 * be inserted into inactive list.
                 */
                cached_pages_list_append_node(
                        &page_cache.inactive_list, p, INACTIVE_LIST);
                if (page_cache.inactive_list.size > INACTIVE_LIST_MAX) {
                        /*
                         * if inactive list is full,
                         * evict head unpinned page.
                         */
                        evict_head_page(&page_cache.inactive_list,
                                        INACTIVE_LIST);
                }

                /* Fill it with corresponding contents. */
                page_cache.user_func.file_read(
                        p->content, file_page_idx, pce->private_data);

                temp_is_new = true;
        } else {
                /* Cache hit. */
#ifdef TEST_COUNT_PAGE_CACHE
                count.hit = count.hit + 1;
#endif
                page_cache_debug(
                        "[find_or_new_page] page cache %d:%d found in %d\n",
                        pce->host_idx,
                        file_page_idx,
                        p->in_which_list);
                temp_is_new = false;
        }

        if (is_new)
                *is_new = temp_is_new;
        return p;

fail:
        if (p)
                flush_and_free_page(p);
        return NULL;
}

/* The caller should hold per page_cache mutex lock! */
void write_back_all_pages(void)
{
        struct cached_page *p;

        /* Write back pages in active list. */
        for_each_in_list (p,
                          struct cached_page,
                          two_list_node,
                          &page_cache.active_list.queue) {
                pthread_rwlock_rdlock(&p->page_rwlock);
                flush_single_page(p);
                pthread_rwlock_unlock(&p->page_rwlock);
        }
        /* Write back pages in inactive list. */
        for_each_in_list (p,
                          struct cached_page,
                          two_list_node,
                          &page_cache.inactive_list.queue) {
                pthread_rwlock_rdlock(&p->page_rwlock);
                flush_single_page(p);
                pthread_rwlock_unlock(&p->page_rwlock);
        }
        /* Write back pages in pinned pages list. */
        for_each_in_list (p,
                          struct cached_page,
                          pinned_pages_node,
                          &page_cache.pinned_pages_list.queue) {
                pthread_rwlock_rdlock(&p->page_rwlock);
                flush_single_page(p);
                pthread_rwlock_unlock(&p->page_rwlock);
        }
}

/* Write back all dirty pages periodically. */
void *write_back_routine(void *args)
{
        struct timespec ts;

        while (1) {
                pthread_mutex_lock(&page_cache.page_cache_lock);
                if (page_cache.cache_strategy == WRITE_BACK) {
                        page_cache_debug(
                                "[write_back_routine] write back routine started.\n");
                        write_back_all_pages();
                        page_cache_debug(
                                "[write_back_routine] write back routine completed.\n");
                }
                pthread_mutex_unlock(&page_cache.page_cache_lock);

                ts.tv_sec = WRITE_BACK_CYCLE;
                ts.tv_nsec = 0;
                nanosleep(&ts, NULL);
        }

        return NULL;
}

/* +++++++++++++++++++++++++ Exposed Functions+++++++++++++++++++++++++++ */

void fs_page_cache_init(PAGE_CACHE_STRATEGY strategy,
                        struct user_defined_funcs *uf)
{
        pthread_t thread;

        memcpy(&page_cache.user_func, uf, sizeof(*uf));

        cached_pages_list_init(&page_cache.active_list);
        cached_pages_list_init(&page_cache.inactive_list);
        cached_pages_list_init(&page_cache.pinned_pages_list);

        page_cache.cache_strategy = strategy;
        pthread_create(&thread, 0, write_back_routine, NULL);

        pthread_mutex_init(&page_cache.page_cache_lock, NULL);

        page_cache_debug("fs page cache init finished.\n");
}

struct page_cache_entity_of_inode *
new_page_cache_entity_of_inode(ino_t host_idx, void *private_data)
{
        struct page_cache_entity_of_inode *pce;

        pce = (struct page_cache_entity_of_inode *)calloc(
                1, sizeof(struct page_cache_entity_of_inode));
        if (pce == NULL)
                goto out;

        pce->host_idx = host_idx;
        pce->private_data = private_data;

        cached_pages_list_init(&pce->pages);
        init_radix(&pce->idx2page);

out:
        return pce;
}

int page_cache_switch_strategy(PAGE_CACHE_STRATEGY new_strategy)
{
        if (page_cache.cache_strategy == new_strategy)
                return 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        if (page_cache.cache_strategy == WRITE_BACK) {
                /* Write back all the pages. */
                write_back_all_pages();
        }

        page_cache_debug(
                "[page_cache_switch_strategy] switch cache strategy from %d to %d\n",
                page_cache.cache_strategy,
                new_strategy);
        page_cache.cache_strategy = new_strategy;

        pthread_mutex_unlock(&page_cache.page_cache_lock);

        return 0;
}

int page_cache_check_page(struct page_cache_entity_of_inode *pce,
                          pidx_t file_page_idx)
{
        struct cached_page *p;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        /* Find corresponding page from pce list. */
        p = get_node_from_page_cache_entity(pce, file_page_idx);

        pthread_mutex_unlock(&page_cache.page_cache_lock);

        if (p == NULL)
                return 0;
        else
                return 1;
}

/* Only boost_page in page_cache_get_block_or_page. */
void page_cache_put_block_or_page(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx, int page_block_idx,
                                  PAGE_CACHE_OPERATION_TYPE op_type)
{
        struct cached_page *p;
        bool is_new;

        BUG_ON(page_block_idx < -1 || page_block_idx >= BLOCK_PER_PAGE);
        BUG_ON(op_type != READ && op_type != WRITE);

        pthread_mutex_lock(&page_cache.page_cache_lock);

        p = find_or_new_page(pce, file_page_idx, &is_new);
        if (p == NULL)
                goto out;

        /* Target page must be existed. */
        BUG_ON(is_new == true);

        /* Nothing need to be done when handling read operations. */
        if (op_type == READ) {
                pthread_rwlock_unlock(&p->page_rwlock);
                goto out;
        }

        /* Handling write operation. */
        switch (page_cache.cache_strategy) {
        case DIRECT:
                /* Directly write block or page to disk. */
                if (page_block_idx != -1) {
                        set_block_or_page_dirty(p, page_block_idx, true);
                        if (flush_single_block(p, page_block_idx) != 0)
                                BUG("[pc_set_block_or_page] file_write failed.\n");
                } else {
                        set_block_or_page_dirty(p, -1, true);
                        if (flush_single_page(p) != 0)
                                BUG("[pc_set_block_or_page] file_write failed.\n");
                }

                /* Must unlock before freeing. */
                pthread_rwlock_unlock(&p->page_rwlock);

                /*
                 * Invalidate and free outdated cache.
                 * There won't be any use-after-free error because
                 * page_cache_lock protect us from this error.
                 */
                flush_and_free_page(p);
                break;

        case WRITE_BACK:
                /* Mark this block or page as dirty. */
                set_block_or_page_dirty(p, page_block_idx, true);

                pthread_rwlock_unlock(&p->page_rwlock);
                break;

        case WRITE_THROUGH:
                /* Directly write block or page to disk. */
                if (page_block_idx != -1) {
                        set_block_or_page_dirty(p, page_block_idx, true);
                        if (flush_single_block(p, page_block_idx) != 0)
                                BUG("[pc_set_block_or_page] file_write failed.\n");
                } else {
                        set_block_or_page_dirty(p, -1, true);
                        if (flush_single_page(p) != 0)
                                BUG("[pc_set_block_or_page] file_write failed.\n");
                }

                pthread_rwlock_unlock(&p->page_rwlock);
                break;
        }

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
}

/* Only boost_page when calling page_cache_get_block_or_page. */
char *page_cache_get_block_or_page(struct page_cache_entity_of_inode *pce,
                                   pidx_t file_page_idx, int page_block_idx,
                                   PAGE_CACHE_OPERATION_TYPE op_type)
{
        struct cached_page *p;
        bool is_new;
        char *buf;

        BUG_ON(page_block_idx < -1 || page_block_idx >= BLOCK_PER_PAGE);
        BUG_ON(op_type != READ && op_type != WRITE);

        pthread_mutex_lock(&page_cache.page_cache_lock);

        p = find_or_new_page(pce, file_page_idx, &is_new);
        if (p == NULL) {
                buf = NULL;
                goto out;
        }

        /* Grab read or write lock of this page. */
        if (op_type == READ)
                pthread_rwlock_rdlock(&p->page_rwlock);
        else
                pthread_rwlock_wrlock(&p->page_rwlock);

        /* Read from corresponding cached page. */
        if (page_block_idx != -1)
                buf = p->content + page_block_idx * CACHED_BLOCK_SIZE;
        else
                buf = p->content;

        /* New allocated pages should not be boosted. */
        if (is_new == false)
                boost_page(p);

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return buf;
}

int page_cache_flush_block_or_page(struct page_cache_entity_of_inode *pce,
                                   pidx_t file_page_idx, int page_block_idx)
{
        struct cached_page *p;
        int ret = 0;

        BUG_ON(page_block_idx < -1 || page_block_idx >= BLOCK_PER_PAGE);

        pthread_mutex_lock(&page_cache.page_cache_lock);

        p = get_node_from_page_cache_entity(pce, file_page_idx);
        if (p == NULL) {
                page_cache_debug("Corresponding page cache does not exist!\n");
                ret = -1;
                goto out;
        }

        pthread_rwlock_rdlock(&p->page_rwlock);
        if (page_block_idx != -1)
                flush_single_block(p, page_block_idx);
        else
                flush_single_page(p);

out:
        if (p)
                pthread_rwlock_unlock(&p->page_rwlock);
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_flush_pages_of_inode(struct page_cache_entity_of_inode *pce)
{
        struct cached_page *p;
        int ret = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        page_cache_debug(
                "[page_cache_flush_pages_of_inode] write back inode pages started.\n");

        /* Write back pages that belongs to an inode. */
        for_each_in_list (
                p, struct cached_page, inode_pages_node, &pce->pages.queue) {
                pthread_rwlock_rdlock(&p->page_rwlock);
                if (flush_single_page(p) != 0) {
                        ret = -1;
                        goto fail_unlock;
                }
                pthread_rwlock_unlock(&p->page_rwlock);
        }

        page_cache_debug(
                "[page_cache_flush_pages_of_inode] write back inode pages finished.\n");
        goto out;

fail_unlock:
        pthread_rwlock_unlock(&p->page_rwlock);
out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_flush_all_pages(void)
{
        pthread_mutex_lock(&page_cache.page_cache_lock);

        page_cache_debug(
                "[page_cache_flush_all_pages] flush all pages started.\n");

        write_back_all_pages();

        page_cache_debug(
                "[page_cache_flush_all_pages] flush all pages finished.\n");

        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return 0;
}

int page_cache_pin_single_page(struct page_cache_entity_of_inode *pce,
                               pidx_t file_page_idx)
{
        struct cached_page *p;
        int ret = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        /* Check max pinned pages. */
        if (page_cache.pinned_pages_list.size == MAX_PINNED_PAGE) {
                page_cache_debug("pinned pages number reach limits.\n");
                ret = -1;
                goto out;
        }

        /* Find target page, if not found, create one. */
        p = find_or_new_page(pce, file_page_idx, NULL);
        if (p == NULL) {
                ret = -1;
                goto out;
        }

        /* Remove this page from two lists. */
        if (p->in_which_list == ACTIVE_LIST) {
                cached_pages_list_delete_node(
                        &page_cache.active_list, p, ACTIVE_LIST);
        } else if (p->in_which_list == INACTIVE_LIST) {
                cached_pages_list_delete_node(
                        &page_cache.inactive_list, p, INACTIVE_LIST);
        } else if (p->in_which_list == PINNED_PAGES_LIST) {
                /* Page already pinned. */
                ret = 0;
                goto out;
        } else {
                BUG("Invalid list type.\n");
                ret = -1;
                goto out;
        }

        /* Insert this page to pinned pages list. */
        cached_pages_list_append_node(
                &page_cache.pinned_pages_list, p, PINNED_PAGES_LIST);

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_unpin_single_page(struct page_cache_entity_of_inode *pce,
                                 pidx_t file_page_idx)
{
        struct cached_page *p;
        int ret = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        /* Ensuring target page is already existed. */
        p = get_node_from_page_cache_entity(pce, file_page_idx);
        if (p == NULL) {
                page_cache_debug("page not found.\n");
                ret = -1;
                goto out;
        }

        /* Ensuring target page is already pinned. */
        if (p->in_which_list != PINNED_PAGES_LIST) {
                page_cache_debug("page not pinned.\n");
                ret = -1;
                goto out;
        }

        /*
         * Because we are unlikely to access this page again after we unpin it,
         * we just flush and free this page.
         */
        flush_and_free_page(p);

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_pin_multiple_pages(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx, u64 page_num)
{
        u64 i;

        for (i = file_page_idx; i < file_page_idx + page_num; ++i)
                if (page_cache_pin_single_page(pce, i) != 0)
                        return -1;

        return 0;
}

int page_cache_unpin_multiple_pages(struct page_cache_entity_of_inode *pce,
                                    pidx_t file_page_idx, u64 page_num)
{
        u64 i;

        for (i = file_page_idx; i < file_page_idx + page_num; ++i)
                if (page_cache_unpin_single_page(pce, i) != 0)
                        return -1;

        return 0;
}

int page_cache_evict_single_page(struct page_cache_entity_of_inode *pce,
                                 pidx_t file_page_idx)
{
        struct cached_page *p;
        int ret = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        /* Ensuring target page is already existing. */
        p = get_node_from_page_cache_entity(pce, file_page_idx);
        if (p == NULL) {
                page_cache_debug(
                        "[page_cache_evict_single_page] page does not exist.\n");
                ret = -1;
                goto out;
        }

        page_cache_debug("Evict %d:%d in list %d\n",
                         p->owner->host_idx,
                         p->file_page_idx,
                         p->in_which_list);
        flush_and_free_page(p);

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_evict_pages_of_inode(struct page_cache_entity_of_inode *pce)
{
        struct cached_page **p_array;
        struct cached_page *p;
        int queue_size, i = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        queue_size = pce->pages.size;
        p_array = (struct cached_page **)calloc(pce->pages.size,
                                                sizeof(struct cached_page *));
        BUG_ON(p_array == NULL);

        /* Find all target pages. */
        for_each_in_list (
                p, struct cached_page, inode_pages_node, &pce->pages.queue)
                p_array[i++] = p;
        BUG_ON(i != queue_size);

        for (i = 0; i < queue_size; ++i) {
                page_cache_debug("Evict %d:%d in list %d\n",
                                 p_array[i]->owner->host_idx,
                                 p_array[i]->file_page_idx,
                                 p_array[i]->in_which_list);
                flush_and_free_page(p_array[i]);
        }

        free(p_array);

        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return 0;
}

int page_cache_delete_single_page(struct page_cache_entity_of_inode *pce,
                                  pidx_t file_page_idx)
{
        struct cached_page *p;
        int ret = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        /* Ensuring target page is already existing. */
        p = get_node_from_page_cache_entity(pce, file_page_idx);
        if (p == NULL) {
                page_cache_debug(
                        "[page_cache_delete_single_page] page does not exist.\n");
                ret = -1;
                goto out;
        }

        page_cache_debug("Delete %d:%d in list %d\n",
                         p->owner->host_idx,
                         p->file_page_idx,
                         p->in_which_list);
        free_page(p);

out:
        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return ret;
}

int page_cache_delete_pages_of_inode(struct page_cache_entity_of_inode *pce)
{
        struct cached_page **p_array;
        struct cached_page *p;
        int queue_size, i = 0;

        pthread_mutex_lock(&page_cache.page_cache_lock);

        queue_size = pce->pages.size;
        p_array = (struct cached_page **)calloc(pce->pages.size,
                                                sizeof(struct cached_page *));
        BUG_ON(p_array == NULL);

        /* Find all target pages. */
        for_each_in_list (
                p, struct cached_page, inode_pages_node, &pce->pages.queue)
                p_array[i++] = p;
        BUG_ON(i != queue_size);

        for (i = 0; i < queue_size; ++i) {
                page_cache_debug("Delete %d:%d in list %d\n",
                                 p_array[i]->owner->host_idx,
                                 p_array[i]->file_page_idx,
                                 p_array[i]->in_which_list);
                free_page(p_array[i]);
        }

        free(p_array);

        pthread_mutex_unlock(&page_cache.page_cache_lock);
        return 0;
}
