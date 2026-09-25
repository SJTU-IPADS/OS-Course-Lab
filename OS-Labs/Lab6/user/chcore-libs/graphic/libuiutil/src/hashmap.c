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

#include <hashmap.h>

#include <malloc.h>

struct hashnode {
        struct hashnode *next;
        struct hashnode **pprev;
        u64 key;
        void *val;
};

struct hashbucket {
        struct hashnode *next;
};

struct hashmap {
        u32 size;
        struct hashbucket *table;
};

/* Overlap ! */
static void hashbucket_add(struct hashbucket *bucket, u64 key, void *val)
{
        struct hashnode *node = bucket->next;
        while (node != NULL) {
                if (node->key == key)
                        break;
                node = node->next;
        }
        if (node == NULL) {
                node = (struct hashnode *)malloc(sizeof(*node));
                node->next = bucket->next;
                node->pprev = &bucket->next;
                if (bucket->next != NULL)
                        bucket->next->pprev = &node->next;
                bucket->next = node;
                node->key = key;
                node->val = val;
        } else {
                node->val = val;
        }
}

static void del_hashbucket(struct hashbucket *bucket)
{
        struct hashnode *prev, *node;
        if (bucket->next == NULL)
                return;
        prev = bucket->next;
        node = prev->next;
        while (node != NULL) {
                free(prev);
                prev = node;
                node = node->next;
        }
        free(prev);
}

static void *hashbucket_del(struct hashbucket *bucket, u64 key)
{
        struct hashnode *node = bucket->next;
        void *val;
        while (node != NULL) {
                if (node->key == key) {
                        val = node->val;
                        if (node->next != NULL)
                                node->next->pprev = node->pprev;
                        *node->pprev = node->next;
                        free(node);
                        return val;
                }
        }
        return NULL;
}

static void *hashbucket_get(struct hashbucket *bucket, u64 key)
{
        struct hashnode *node = bucket->next;
        while (node != NULL) {
                if (node->key == key)
                        return node->val;
        }
        return NULL;
}

struct hashmap *create_hashmap(u32 size)
{
        struct hashmap *hm;
        if (size == 0)
                return NULL;
        hm = (struct hashmap *)malloc(sizeof(*hm));
        hm->size = size;
        hm->table = (struct hashbucket *)calloc(1, sizeof(*hm->table) * size);
        return hm;
}

void del_hashmap(struct hashmap *hm)
{
        u32 i;
        if (hm == NULL)
                return;
        if (hm->table != NULL) {
                for (i = 0; i < hm->size; ++i) {
                        del_hashbucket(hm->table + i);
                }
                free(hm->table);
        }
        free(hm);
}

inline void hashmap_add(struct hashmap *hm, u64 key, void *val)
{
        hashbucket_add(hm->table + (key % hm->size), key, val);
}

inline void *hashmap_del(struct hashmap *hm, u64 key)
{
        return hashbucket_del(hm->table + (key % hm->size), key);
}

inline void *hashmap_get(struct hashmap *hm, u64 key)
{
        return hashbucket_get(hm->table + (key % hm->size), key);
}

bool hashmap_empty(struct hashmap *hm)
{
        for (u32 i = 0; i < hm->size; ++i) {
                if (hm->table[i].next != NULL)
                        return false;
        }
        return true;
}
