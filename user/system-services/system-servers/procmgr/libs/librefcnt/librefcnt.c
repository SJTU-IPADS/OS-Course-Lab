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

#include "librefcnt.h"
#include <malloc.h>
#include <pthread.h>

#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#define FETCH_AND_ADD_SUPPORT
#endif

struct object {
        volatile unsigned long refcnt;
#ifndef FETCH_AND_ADD_SUPPORT
        pthread_mutex_t lock;
#endif
        obj_deinit_t obj_deinit;
        u64 opaque[];
};

#define container_of(ptr, type, field) \
        ((type *)((char *)(ptr) - (uintptr_t)(&(((type *)(0))->field))))

inline unsigned long fetch_and_add(struct object *p, unsigned long c)
{
#ifdef FETCH_AND_ADD_SUPPORT
        return __sync_fetch_and_add(&p->refcnt, c);
#else
        unsigned long origin;
        pthread_mutex_lock(&p->lock);
        origin = p->refcnt;
        p->refcnt += c;
        pthread_mutex_unlock(&p->lock);
        return origin;
#endif
}
inline unsigned long fetch_and_sub(struct object *p, unsigned long c)
{
#ifdef FETCH_AND_ADD_SUPPORT
        return __sync_fetch_and_sub(&p->refcnt, c);
#else
        unsigned long origin;
        pthread_mutex_lock(&p->lock);
        origin = p->refcnt;
        p->refcnt -= c;
        pthread_mutex_unlock(&p->lock);
        return origin;
#endif
}


void *obj_alloc(size_t size, obj_deinit_t obj_deinit)
{
        struct object *object;

        object = malloc(size + sizeof(*object));
        if (object == NULL) {
                return NULL;
        }

        object->obj_deinit = obj_deinit;
        object->refcnt = 1;
#ifndef FETCH_AND_ADD_SUPPORT
        pthread_mutex_init(&object->lock, NULL);
#endif
        return object->opaque;
}

void *obj_get(void *obj)
{
        struct object *object;

        object = container_of(obj, struct object, opaque);
        fetch_and_add(object, 1);

        return object->opaque;
}

void obj_put(void *obj)
{
        struct object *object;
        unsigned long old_refcnt;

        object = container_of(obj, struct object, opaque);
        old_refcnt = fetch_and_sub(object, 1);
        if (old_refcnt == 1) {
                if (object->obj_deinit) {
                        object->obj_deinit(obj);
                }
                free(object);
        }
}

/* for debugging */
int obj_cnt(void *obj)
{
        struct object *object;

        object = container_of(obj, struct object, opaque);

        return object->refcnt;
}
