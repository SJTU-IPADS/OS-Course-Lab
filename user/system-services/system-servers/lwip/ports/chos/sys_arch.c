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

/* CHCore */
#include "lwip/debug.h"
#include "lwip/tcpip.h"

#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "lwip/def.h"

#ifdef LWIP_UNIX_MACH
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/sys.h"

static void get_monotonic_time(struct timespec *ts)
{
	clock_gettime(CLOCK_MONOTONIC, ts);
}

static struct sys_thread *threads = NULL;
static pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;

struct sys_mbox_msg {
	struct sys_mbox_msg *next;
	void *msg;
};

#define SYS_MBOX_SIZE 128

struct sys_mbox {
	int first, last;
	void *msgs[SYS_MBOX_SIZE];
	struct sys_sem *not_empty;
	struct sys_sem *not_full;
	struct sys_sem *mutex;
	int wait_send;
};

struct sys_sem {
	volatile int c;
	pthread_condattr_t condattr;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

struct sys_mutex {
	pthread_mutex_t mutex;
};

struct sys_thread {
	struct sys_thread *next;
	pthread_t pthread;
};

static pthread_mutex_t lwprot_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t lwprot_thread = (pthread_t) 0xDEAD;
static int lwprot_count = 0;

static struct sys_sem *sys_sem_new_internal(u8_t count);
static void sys_sem_free_internal(struct sys_sem *sem);

static struct sys_thread *introduce_thread(pthread_t id)
{
	struct sys_thread *thread;

	thread = (struct sys_thread *)malloc(sizeof(struct sys_thread));
	if (thread != NULL) {
		pthread_mutex_lock(&threads_mutex);
		thread->next = threads;
		thread->pthread = id;
		threads = thread;
		pthread_mutex_unlock(&threads_mutex);
	}
	return thread;
}

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn function,
			    void *arg, int stacksize, int prio)
{
	int code;
	pthread_t tmp;
	struct sys_thread *st = NULL;

	LWIP_UNUSED_ARG(name);
	LWIP_UNUSED_ARG(stacksize);
	LWIP_UNUSED_ARG(prio);
	code = pthread_create(&tmp, NULL, (void *(*)(void *))function, arg);
	if (0 == code) {
		st = introduce_thread(tmp);
	}
	if (NULL == st) {
		LWIP_DEBUGF(SYS_DEBUG,
			    ("sys_thread_new: pthread_create %d, st = 0x%lx",
			     code, (unsigned long)st));
		abort();
	}
	return st;
}

err_t sys_mbox_new(struct sys_mbox **mb, int size)
{
	struct sys_mbox *mbox;
	LWIP_UNUSED_ARG(size);

	mbox = (struct sys_mbox *)malloc(sizeof(struct sys_mbox));
	if (mbox == NULL) {
		return ERR_MEM;
	}
	mbox->first = mbox->last = 0;
	mbox->not_empty = sys_sem_new_internal(0);
	mbox->not_full = sys_sem_new_internal(0);
	mbox->mutex = sys_sem_new_internal(1);
	mbox->wait_send = 0;

	SYS_STATS_INC_USED(mbox);
	*mb = mbox;
	return ERR_OK;
}

void sys_mbox_free(struct sys_mbox **mb)
{
	if ((mb != NULL) && (*mb != SYS_MBOX_NULL)) {
		struct sys_mbox *mbox = *mb;
		SYS_STATS_DEC(mbox.used);
		sys_arch_sem_wait(&mbox->mutex, 0);
		sys_sem_free_internal(mbox->not_empty);
		sys_sem_free_internal(mbox->not_full);
		sys_sem_free_internal(mbox->mutex);
		mbox->not_empty = mbox->not_full = mbox->mutex = NULL;
		free(mbox);
	}
}

err_t sys_mbox_trypost(struct sys_mbox **mb, void *msg)
{
	u8_t first;
	struct sys_mbox *mbox;
	LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
	mbox = *mb;

	sys_arch_sem_wait(&mbox->mutex, 0);
	LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n",
				(void *)mbox, (void *)msg));
	if ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
		sys_sem_signal(&mbox->mutex);
		return ERR_MEM;
	}
	mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;
	if (mbox->last == mbox->first) {
		first = 1;
	} else {
		first = 0;
	}
	mbox->last++;
	if (first) {
		sys_sem_signal(&mbox->not_empty);
	}
	sys_sem_signal(&mbox->mutex);
	return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t * mbox, void *msg)
{
	return sys_mbox_trypost(mbox, msg);
}

void sys_mbox_post(struct sys_mbox **mb, void *msg)
{
	u8_t first;
	struct sys_mbox *mbox;
	LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
	mbox = *mb;

	sys_arch_sem_wait(&mbox->mutex, 0);
	LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_post: mbox %p msg %p\n", (void *)mbox,
				(void *)msg));
	while ((mbox->last + 1) >= (mbox->first + SYS_MBOX_SIZE)) {
		mbox->wait_send++;
		sys_sem_signal(&mbox->mutex);
		sys_arch_sem_wait(&mbox->not_full, 0);
		sys_arch_sem_wait(&mbox->mutex, 0);
		mbox->wait_send--;
	}
	mbox->msgs[mbox->last % SYS_MBOX_SIZE] = msg;
	if (mbox->last == mbox->first) {
		first = 1;
	} else {
		first = 0;
	}
	mbox->last++;
	if (first) {
		sys_sem_signal(&mbox->not_empty);
	}
	sys_sem_signal(&mbox->mutex);
}

u32_t sys_arch_mbox_tryfetch(struct sys_mbox **mb, void **msg)
{
	struct sys_mbox *mbox;
	LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
	mbox = *mb;

	sys_arch_sem_wait(&mbox->mutex, 0);
	if (mbox->first == mbox->last) {
		sys_sem_signal(&mbox->mutex);
		return SYS_MBOX_EMPTY;
	}
	if (msg != NULL) {
		LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_tryfetch: mbox %p msg %p\n",
					(void *)mbox, *msg));
		*msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
	} else {
		LWIP_DEBUGF(SYS_DEBUG,
			    ("sys_mbox_tryfetch: mbox %p, null msg\n",
			     (void *)mbox));
	}
	mbox->first++;
	if (mbox->wait_send) {
		sys_sem_signal(&mbox->not_full);
	}
	sys_sem_signal(&mbox->mutex);
	return 0;
}

u32_t sys_arch_mbox_fetch_wrapper(struct sys_mbox **mb, void **msg, u32_t timeout)
{
	u32_t ret;
	UNLOCK_TCPIP_CORE();
	ret = sys_arch_mbox_fetch(mb, msg, timeout);
	LOCK_TCPIP_CORE();
	return ret;
}

u32_t sys_arch_mbox_fetch(struct sys_mbox **mb, void **msg, u32_t timeout)
{
	u32_t time_needed = 0;
	struct sys_mbox *mbox;
	LWIP_ASSERT("invalid mbox", (mb != NULL) && (*mb != NULL));
	mbox = *mb;


	/* The mutex pthread_mutex_lock is quick so we don't bother with the timeout
	   stuff here. */
	sys_arch_sem_wait(&mbox->mutex, 0);
	while (mbox->first == mbox->last) {
		sys_sem_signal(&mbox->mutex);

		/* We block while waiting for a mail to arrive in the mailbox.
		   We must be prepared to timeout. */
		if (timeout != 0) {
			time_needed =
			    sys_arch_sem_wait(&mbox->not_empty, timeout);
			if (time_needed == SYS_ARCH_TIMEOUT) {
				return SYS_ARCH_TIMEOUT;
			}
		} else {
			sys_arch_sem_wait(&mbox->not_empty, 0);
		}
		sys_arch_sem_wait(&mbox->mutex, 0);
	}
	if (msg != NULL) {
		LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p msg %p\n",
					(void *)mbox, *msg));
		*msg = mbox->msgs[mbox->first % SYS_MBOX_SIZE];
	} else {
		LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_fetch: mbox %p, null msg\n",
					(void *)mbox));
	}
	mbox->first++;
	if (mbox->wait_send) {
		sys_sem_signal(&mbox->not_full);
	}
	sys_sem_signal(&mbox->mutex);

	return time_needed;
}

/*-----------------------------------------------------------------------------------*/
/* Semaphore */
static struct sys_sem *sys_sem_new_internal(u8_t count)
{
	struct sys_sem *sem;

	sem = (struct sys_sem *)malloc(sizeof(struct sys_sem));
	if (sem != NULL) {
		sem->c = count;
		pthread_mutex_init(&(sem->mutex), NULL);
		pthread_condattr_init(&(sem->condattr));
		pthread_cond_init(&(sem->cond), &(sem->condattr));
		pthread_mutex_init(&(sem->mutex), NULL);
	}
	return sem;
}

err_t sys_sem_new(struct sys_sem **sem, u8_t count)
{
	SYS_STATS_INC_USED(sem);
	*sem = sys_sem_new_internal(count);
	if (*sem == NULL) {
		return ERR_MEM;
	}
	return ERR_OK;
}

u32_t cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex, u32_t timeout)
{
	struct timespec rtime1, rtime2, ts;
	int ret;

	if (timeout == 0) {
		pthread_cond_wait(cond, mutex);
		return 0;
	}

	/* Get a timestamp and add the timeout value. */
	get_monotonic_time(&rtime1);
	ts.tv_sec = rtime1.tv_sec + timeout / 1000L;
	ts.tv_nsec = rtime1.tv_nsec + (timeout % 1000L) * 1000000L;
	if (ts.tv_nsec >= 1000000000L) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000L;
	}

	ret = pthread_cond_timedwait(cond, mutex, &ts);
	if (ret == ETIMEDOUT) {
		return SYS_ARCH_TIMEOUT;
	}

	/* Calculate for how long we waited for the cond. */
	get_monotonic_time(&rtime2);
	ts.tv_sec = rtime2.tv_sec - rtime1.tv_sec;
	ts.tv_nsec = rtime2.tv_nsec - rtime1.tv_nsec;
	if (ts.tv_nsec < 0) {
		ts.tv_sec--;
		ts.tv_nsec += 1000000000L;
	}
	return (u32_t) (ts.tv_sec * 1000L + ts.tv_nsec / 1000000L);
}

u32_t sys_arch_sem_wait(struct sys_sem **s, u32_t timeout)
{
	u32_t time_needed = 0;
	struct sys_sem *sem;
	LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
	sem = *s;

	pthread_mutex_lock(&(sem->mutex));
	while (sem->c <= 0) {
		if (timeout > 0) {
			time_needed =
			    cond_wait(&(sem->cond), &(sem->mutex), timeout);
			if (time_needed == SYS_ARCH_TIMEOUT) {
				pthread_mutex_unlock(&(sem->mutex));
				return SYS_ARCH_TIMEOUT;
			}
		} else {
			cond_wait(&(sem->cond), &(sem->mutex), 0);
		}
	}
	sem->c--;
	pthread_mutex_unlock(&(sem->mutex));
	return (u32_t) time_needed;
}

void sys_sem_signal(struct sys_sem **s)
{
	struct sys_sem *sem;

	LWIP_ASSERT("invalid sem", (s != NULL) && (*s != NULL));
	sem = *s;
	pthread_mutex_lock(&(sem->mutex));
	sem->c++;
	if (sem->c > 1)
		sem->c = 1;
	pthread_cond_broadcast(&(sem->cond));
	pthread_mutex_unlock(&(sem->mutex));
}

static void sys_sem_free_internal(struct sys_sem *sem)
{
	pthread_cond_destroy(&(sem->cond));
	pthread_condattr_destroy(&(sem->condattr));
	pthread_mutex_destroy(&(sem->mutex));
	free(sem);
}

void sys_sem_free(struct sys_sem **sem)
{
	if ((sem != NULL) && (*sem != SYS_SEM_NULL)) {
		SYS_STATS_DEC(sem.used);
		sys_sem_free_internal(*sem);
	}
}

err_t sys_mutex_new(struct sys_mutex **mutex)
{
	struct sys_mutex *mtx;

	mtx = (struct sys_mutex *)malloc(sizeof(struct sys_mutex));
	if (mtx != NULL) {
		pthread_mutex_init(&(mtx->mutex), NULL);
		*mutex = mtx;
		return ERR_OK;
	} else {
		return ERR_MEM;
	}
}

/** pthread_mutex_lock a mutex
 * @param mutex the mutex to pthread_mutex_lock */
void sys_mutex_lock(struct sys_mutex **mutex)
{
	pthread_mutex_lock(&((*mutex)->mutex));
}

/** pthread_mutex_unlock a mutex
 * @param mutex the mutex to pthread_mutex_unlock */
void sys_mutex_unlock(struct sys_mutex **mutex)
{
	pthread_mutex_unlock(&((*mutex)->mutex));
}

/** Delete a mutex
 * @param mutex the mutex to delete */
void sys_mutex_free(struct sys_mutex **mutex)
{
	pthread_mutex_destroy(&((*mutex)->mutex));
	free(*mutex);
}

u32_t sys_now(void)
{
	struct timespec ts;

	get_monotonic_time(&ts);
	return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

u32_t sys_jiffies(void)
{
	struct timespec ts;

	get_monotonic_time(&ts);
	return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

/*-----------------------------------------------------------------------------------*/
/* Init */

void sys_init(void)
{
}

/*-----------------------------------------------------------------------------------*/
/* Critical section */
#if SYS_LIGHTWEIGHT_PROT
/** sys_prot_t sys_arch_protect(void)

  This optional function does a "fast" critical region protection and returns
  the previous protection level. This function is only called during very short
  critical regions. An embedded system which supports ISR-based drivers might
  want to implement this function by disabling interrupts. Task-based systems
  might want to implement this by using a mutex or disabling tasking. This
  function should support recursive calls from the same task or interrupt. In
  other words, sys_arch_protect() could be called while already protected. In
  that case the return value indicates that it is already protected.

  sys_arch_protect() is only required if your port is supporting an operating
  system.
  */
sys_prot_t sys_arch_protect(void)
{
	/* Note that for the UNIX port, we are using a lightweight mutex, and
	 * our own counter (which is locked by the mutex). The return code is
	 * not actually used. */
	if (lwprot_thread != pthread_self()) {
		/* We are locking the mutex where it has not been locked before
		 * * or is being locked by another thread */
		pthread_mutex_lock(&lwprot_mutex);
		lwprot_thread = pthread_self();
		lwprot_count = 1;
	} else
		/* It is already locked by THIS thread */
		lwprot_count++;
	return 0;
}

/** void sys_arch_unprotect(sys_prot_t pval)

  This optional function does a "fast" set of critical region protection to the
  value specified by pval. See the documentation for sys_arch_protect() for
  more information. This function is only required if your port is supporting
  an operating system.
  */
void sys_arch_unprotect(sys_prot_t pval)
{
	LWIP_UNUSED_ARG(pval);
	if (lwprot_thread == pthread_self()) {
		lwprot_count--;
		if (lwprot_count == 0) {
			lwprot_thread = (pthread_t) 0xDEAD;
			pthread_mutex_unlock(&lwprot_mutex);
		}
	}
}
#endif /* SYS_LIGHTWEIGHT_PROT */
