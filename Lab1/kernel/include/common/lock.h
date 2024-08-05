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

#include <common/types.h>

/* Simple/Compact ticket/rwlock impl */

struct lock {
	volatile u64 slock;
};

struct rwlock {
	volatile u32 lock;
};

#define DEFINE_SPINLOCK(x)	struct lock x = { .slock = 0 }
#define DEFINE_RWLOCK(x)	struct rwlock x = { .lock = 0 }

int lock_init(struct lock *lock);
void lock(struct lock *lock);
/* returns 0 on success, -1 otherwise */
int try_lock(struct lock *lock);
void unlock(struct lock *lock);
int is_locked(struct lock *lock);

/* Global locks */
extern struct lock big_kernel_lock;

int rwlock_init(struct rwlock *rwlock);
void read_lock(struct rwlock *rwlock);
void write_lock(struct rwlock *rwlock);
/* read/write_try_lock return 0 on success, -1 otherwise */
int read_try_lock(struct rwlock *rwlock);
int write_try_lock(struct rwlock *rwlock);
void read_unlock(struct rwlock *rwlock);
void write_unlock(struct rwlock *rwlock);
void rw_downgrade(struct rwlock *rwlock);

#endif /* COMMON_LOCK_H */