#pragma once

#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <sys/time.h>

// Spinlock
typedef int spinlock_t;
#define SPIN_INIT() 0

static inline int atomic_xchg(volatile int *addr, int newval)
{
    int result;
    asm volatile("lock xchg %0, %1" : "+m"(*addr), "=a"(result) : "1"(newval) : "memory");
    return result;
}

void spin_lock(spinlock_t *lk);

void spin_unlock(spinlock_t *lk);

// Mutex
typedef pthread_mutex_t mutex_t;
#define MUTEX_INIT() PTHREAD_MUTEX_INITIALIZER
#define mutex_init(mutex) pthread_mutex_init(mutex, NULL)
#define mutex_lock pthread_mutex_lock
#define mutex_unlock pthread_mutex_unlock

// Conditional Variable
typedef pthread_cond_t cond_t;
#define COND_INIT() PTHREAD_COND_INITIALIZER
#define cond_wait pthread_cond_wait
#define cond_broadcast pthread_cond_broadcast
#define cond_signal pthread_cond_signal

// Semaphore
#define P sem_wait
#define V sem_post
#define SEM_INIT(sem, val) sem_init(sem, 0, val)
