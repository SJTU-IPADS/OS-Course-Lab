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
#include <chcore/type.h>
#include <chcore/defs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define container_of(ptr, type, field) \
        ((type *)((char *)(ptr) - (uintptr_t)(&(((type *)(0))->field))))

#define container_of_safe(ptr, type, field)                     \
        ({                                                      \
                __typeof__(ptr) __ptr = (ptr);                  \
                type *__obj = container_of(__ptr, type, field); \
                (__ptr ? __obj : NULL);                         \
        })

struct list_head {
        struct list_head *prev;
        struct list_head *next;
};

static inline void init_list_head(struct list_head *list)
{
        list->next = list;
        list->prev = list;
}

static inline void list_add(struct list_head *new_node, struct list_head *head)
{
        new_node->next = head->next;
        new_node->prev = head;
        head->next->prev = new_node;
        head->next = new_node;
}

static inline void list_append(struct list_head *new_node,
                               struct list_head *head)
{
        struct list_head *tail = head->prev;
        return list_add(new_node, tail);
}

static inline void list_del(struct list_head *node)
{
        node->prev->next = node->next;
        node->next->prev = node->prev;
}

static inline bool list_empty(const struct list_head *head)
{
        return (head->prev == head && head->next == head);
}

#define next_container_of_safe(obj, type, field)               \
        ({                                                     \
                __typeof__(obj) __obj = (obj);                 \
                (__obj ? container_of_safe(                    \
                         ((__obj)->field).next, type, field) : \
                         NULL);                                \
        })

#define list_entry(ptr, type, field) container_of(ptr, type, field)

#if 0
#define for_each_node_in_list(node, head) \
        for (node = iter->next; node != head; node = node->next)
#endif

#define for_each_in_list(elem, type, field, head)            \
        for (elem = container_of((head)->next, type, field); \
             &((elem)->field) != (head);                     \
             elem = container_of(((elem)->field).next, type, field))

#define for_each_in_list_reverse(elem, type, field, head)    \
        for (elem = container_of((head)->prev, type, field); \
             &((elem)->field) != (head);                     \
             elem = container_of(((elem)->field).prev, type, field))

#define __for_each_in_list_safe(elem, tmp, type, field, head) \
        for (elem = container_of((head)->next, type, field),  \
            tmp = next_container_of_safe(elem, type, field);  \
             &((elem)->field) != (head);                      \
             elem = tmp, tmp = next_container_of_safe(tmp, type, field))

#define for_each_in_list_safe(elem, tmp, field, head) \
        __for_each_in_list_safe (elem, tmp, __typeof__(*elem), field, head)

struct hlist_head {
        struct hlist_node *next;
};
struct hlist_node {
        struct hlist_node *next;
        struct hlist_node **pprev; /* the field that points to this node */
};

static inline void init_hlist_head(struct hlist_head *head)
{
        head->next = NULL;
}
static inline void init_hlist_node(struct hlist_node *node)
{
        node->next = NULL;
        node->pprev = NULL;
}

static inline void hlist_add(struct hlist_node *new_node,
                             struct hlist_head *head)
{
        new_node->next = head->next;
        new_node->pprev = &head->next;
        if (head->next)
                head->next->pprev = &new_node->next;
        head->next = new_node;
}

static inline void hlist_del(struct hlist_node *node)
{
        if (node->next)
                node->next->pprev = node->pprev;
        *node->pprev = node->next;
}

static inline bool hlist_empty(struct hlist_head *head)
{
        return head->next == NULL;
}

#define hlist_entry(ptr, type, field) container_of(ptr, type, field)

#define __for_each_in_hlist(elem, type, field, head)                    \
        for (elem = container_of_safe((head)->next, type, field); elem; \
             elem = elem ? container_of_safe(                           \
                            ((elem)->field).next, type, field) :        \
                           NULL)

#define for_each_in_hlist(elem, field, head) \
        __for_each_in_hlist (elem, __typeof__(*elem), field, head)

#define __for_each_in_hlist_safe(elem, tmp, type, field, head)    \
        for (elem = container_of_safe((head)->next, type, field), \
            tmp = next_container_of_safe(elem, type, field);      \
             elem;                                                \
             elem = tmp, tmp = next_container_of_safe(elem, type, field))

#define for_each_in_hlist_safe(elem, tmp, field, head) \
        __for_each_in_hlist_safe (elem, tmp, __typeof__(*elem), field, head)

#if 0
/*
 * We would better not add prints as the dependencies.
 * Too complicated.
 **/
static inline void kprint_hlist(struct hlist_head *head)
{
	struct hlist_node *node;

	kdebug(" ---- hlist: head=%p ---- \n", head);
	for (node = head->next; node; node = node->next) {
		kdebug("-> %p\n", node);
		if (node == node->next) {
			kdebug("looped list @%p!\n", node);
			BUG("dead loop");
		}
	}
}
#else
static inline void kprint_hlist(struct hlist_head *head)
{
}
#endif

#ifdef __cplusplus
}
#endif
