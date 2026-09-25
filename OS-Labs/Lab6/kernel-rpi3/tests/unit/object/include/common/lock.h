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

#ifndef COMMON_LOCK_H
#define COMMON_LOCK_H

#include <pthread.h>

struct lock {
        pthread_mutex_t mutex;
};

static inline int lock_init(struct lock *lock)
{
        pthread_mutex_init(&lock->mutex, NULL);
        return 0;
}

static inline void lock(struct lock *lock)
{
        pthread_mutex_lock(&lock->mutex);
}

static inline int try_lock(struct lock *lock)
{
        return pthread_mutex_trylock(&lock->mutex);
}

static inline void unlock(struct lock *lock)
{
        pthread_mutex_unlock(&lock->mutex);
}

static inline int is_locked(struct lock *lock)
{
        int r = pthread_mutex_trylock(&lock->mutex);
        if (r == 0)
                pthread_mutex_unlock(&lock->mutex);
        return r != 0;
}

/* FIXME: dummy read write lock just for temporary use */
struct rwlock {
        struct lock lock;
};

static inline int rwlock_init(struct rwlock *rwlock)
{
        return lock_init(&rwlock->lock);
}

static inline void read_lock(struct rwlock *rwlock)
{
        lock(&rwlock->lock);
}

static inline void write_lock(struct rwlock *rwlock)
{
        lock(&rwlock->lock);
}

static inline int read_try_lock(struct rwlock *rwlock)
{
        return try_lock(&rwlock->lock);
}

static inline int write_try_lock(struct rwlock *rwlock)
{
        return try_lock(&rwlock->lock);
}

static inline void read_unlock(struct rwlock *rwlock)
{
        unlock(&rwlock->lock);
}

static inline void write_unlock(struct rwlock *rwlock)
{
        unlock(&rwlock->lock);
}

#endif /* COMMON_LOCK_H */