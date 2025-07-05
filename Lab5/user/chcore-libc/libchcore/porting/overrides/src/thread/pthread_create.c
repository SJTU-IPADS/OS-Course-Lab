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

#define _GNU_SOURCE
#include "pthread_impl.h"
#include "stdio_impl.h"
#include "libc.h"
#include "lock.h"
#include <sys/mman.h>
#include <string.h>
#include <stddef.h>

#include <chcore/proc.h>
#include <chcore/syscall.h>
#include <chcore/ipc.h>
#include <chcore/thread.h>
#include <chcore/pthread.h>

static void dummy_0()
{
}
weak_alias(dummy_0, __acquire_ptc);
weak_alias(dummy_0, __release_ptc);
weak_alias(dummy_0, __pthread_tsd_run_dtors);
weak_alias(dummy_0, __do_orphaned_stdio_locks);
weak_alias(dummy_0, __dl_thread_cleanup);
weak_alias(dummy_0, __membarrier_init);

static int tl_lock_count;
static int tl_lock_waiters;

extern void __thread_exit(void);

#include <debug_lock.h>

void __tl_lock(void)
{
	int tid = __pthread_self()->tid;
	int val = __thread_list_lock;
	if (val == tid) {
		tl_lock_count++;
		return;
	}
#if 0
	while ((val = a_cas(&__thread_list_lock, 0, tid)))
		__wait(&__thread_list_lock, &tl_lock_waiters, val, 0);
#else
	/* 
	 * Chcore does not support clone, thread list can only 
	 * modified by this process. Use private futex here.
	 */
	while ((val = a_cas(&__thread_list_lock, 0, tid)))
		__wait(&__thread_list_lock, &tl_lock_waiters, val, 1);
#endif
}

void __tl_unlock(void)
{
	if (tl_lock_count) {
		tl_lock_count--;
		return;
	}
	a_store(&__thread_list_lock, 0);
#if 0
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 0);
#else
	/* 
	 * Chcore does not support clone, thread list can only 
	 * modified by this process. Use private futex here.
	 */
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 1);
#endif
}

void __tl_sync(pthread_t td)
{
	a_barrier();
	int val = __thread_list_lock;
	if (!val) return;
#if 0
	__wait(&__thread_list_lock, &tl_lock_waiters, val, 0);
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 0);
#else
	/* 
	 * Chcore does not support clone, thread list can only 
	 * modified by this process. Use private futex here.
	 */
	__wait(&__thread_list_lock, &tl_lock_waiters, val, 1);
	if (tl_lock_waiters) __wake(&__thread_list_lock, 1, 1);
#endif
}

_Noreturn void __pthread_exit(void *result)
{
	pthread_t self = __pthread_self();
	sigset_t set;

	self->canceldisable = 1;
	self->cancelasync = 0;
	self->result = result;

	while (self->cancelbuf) {
		void (*f)(void *) = self->cancelbuf->__f;
		void *x = self->cancelbuf->__x;
		self->cancelbuf = self->cancelbuf->__next;
		f(x);
	}

	__pthread_tsd_run_dtors();

	__block_app_sigs(&set);

	/* This atomic potentially competes with a concurrent pthread_detach
	 * call; the loser is responsible for freeing thread resources. */
	int state = a_cas(&self->detach_state, DT_JOINABLE, DT_EXITING);

	if (state==DT_DETACHED && self->map_base) {
		/* Since __unmapself bypasses the normal munmap code path,
		 * explicitly wait for vmlock holders first. This must be
		 * done before any locks are taken, to avoid lock ordering
		 * issues that could lead to deadlock. */
		__vm_wait();
	}

	/* Access to target the exiting thread with syscalls that use
	 * its kernel tid is controlled by killlock. For detached threads,
	 * any use past this point would have undefined behavior, but for
	 * joinable threads it's a valid usage that must be handled.
	 * Signals must be blocked since pthread_kill must be AS-safe. */
	LOCK(self->killlock);

	/* The thread list lock must be AS-safe, and thus depends on
	 * application signals being blocked above. */
	__tl_lock();

	/* If this is the only thread in the list, don't proceed with
	 * termination of the thread, but restore the previous lock and
	 * signal state to prepare for exit to call atexit handlers. */
	if (self->next == self) {
		__tl_unlock();
		UNLOCK(self->killlock);
		self->detach_state = state;
		__restore_sigs(&set);
		exit(0);
	}

	/* At this point we are committed to thread termination. */

	/* Process robust list in userspace to handle non-pshared mutexes
	 * and the detached thread case where the robust list head will
	 * be invalid when the kernel would process it. */
	__vm_lock();
	volatile void *volatile *rp;
	while ((rp=self->robust_list.head) && rp != &self->robust_list.head) {
		pthread_mutex_t *m = (void *)((char *)rp
			- offsetof(pthread_mutex_t, _m_next));
		int waiters = m->_m_waiters;
		int priv = (m->_m_type & 128) ^ 128;
		self->robust_list.pending = rp;
		self->robust_list.head = *rp;
		int cont = a_swap(&m->_m_lock, 0x40000000);
		self->robust_list.pending = 0;
		if (cont < 0 || waiters)
			__wake(&m->_m_lock, 1, priv);
	}
	__vm_unlock();

	__do_orphaned_stdio_locks();
	__dl_thread_cleanup();

	/* Last, unlink thread from the list. This change will not be visible
	 * until the lock is released, which only happens after SYS_exit
	 * has been called, via the exit futex address pointing at the lock.
	 * This needs to happen after any possible calls to LOCK() that might
	 * skip locking if process appears single-threaded. */
	if (!--libc.threads_minus_1) libc.need_locks = -1;
	self->next->prev = self->prev;
	self->prev->next = self->next;
	self->prev = self->next = self;

	if (state==DT_DETACHED && self->map_base) {
		/* Detached threads must block even implementation-internal
		 * signals, since they will not have a stack in their last
		 * moments of existence. */
		__block_all_sigs(&set);

		/* Robust list will no longer be valid, and was already
		 * processed above, so unregister it with the kernel. */
		if (self->robust_list.off)
			__syscall(SYS_set_robust_list, 0, 3*sizeof(long));

		/* CHCORE FIX:
		 * Fix the bug that new thread can't be created after
		 * a pthread_detach call */
		__tl_unlock();

		/* The following call unmaps the thread's stack mapping
		 * and then exits without touching the stack. */
		__unmapself(self->map_base, self->map_size);
	}

	/* Wake any joiner. */
	a_store(&self->detach_state, DT_EXITED);
	__wake(&self->detach_state, 1, 1);

	/* After the kernel thread exits, its tid may be reused. Clear it
	 * to prevent inadvertent use and inform functions that would use
	 * it that it's no longer available. */
	self->tid = 0;
	UNLOCK(self->killlock);
	
	for (;;) __thread_exit();
}

void __do_cleanup_push(struct __ptcb *cb)
{
	struct pthread *self = __pthread_self();
	cb->__next = self->cancelbuf;
	self->cancelbuf = cb;
}

void __do_cleanup_pop(struct __ptcb *cb)
{
	__pthread_self()->cancelbuf = cb->__next;
}

struct start_args {
	void *(*start_func)(void *);
	void *start_arg;
	volatile int control;
	unsigned long sig_mask[_NSIG/8/sizeof(long)];
};

static int start(void *p)
{
	struct start_args *args = p;

#if 0	/* Not support SYS_set_tid_address */
	if (args->control) {
		if (a_cas(&args->control, 1, 2)==1)
			__wait(&args->control, 0, 2, 1);
		if (args->control) {
			__syscall(SYS_set_tid_address, &args->control);
			for (;;) __syscall(SYS_exit, 0);
		}
	}
	__syscall(SYS_rt_sigprocmask, SIG_SETMASK, &args->sig_mask, 0, _NSIG/8);
#endif
	void *result;

#if 0
	fsm_ipc_struct->conn_cap = 0;
	fsm_ipc_struct->server_id = FS_MANAGER;

	lwip_ipc_struct->conn_cap = 0;
	lwip_ipc_struct->server_id = NET_MANAGER;
#endif

	result = args->start_func(args->start_arg);
	__pthread_exit(result);
	return 0;
}

static int start_c11(void *p)
{
	struct start_args *args = p;

#if 0
	fsm_ipc_struct->conn_cap = 0;
	fsm_ipc_struct->server_id = FS_MANAGER;

	lwip_ipc_struct->conn_cap = 0;
	lwip_ipc_struct->server_id = NET_MANAGER;
#endif

	int (*start)(void*) = (int(*)(void*)) args->start_func;
	__pthread_exit((void *)(uintptr_t)start(args->start_arg));
	return 0;
}

#define ROUND(x) (((x)+PAGE_SIZE-1)&-PAGE_SIZE)

/* pthread_key_create.c overrides this */
static volatile size_t dummy = 0;
weak_alias(dummy, __pthread_tsd_size);
static void *dummy_tsd[1] = { 0 };
weak_alias(dummy_tsd, __pthread_tsd_main);

static FILE *volatile dummy_file = 0;
weak_alias(dummy_file, __stdin_used);
weak_alias(dummy_file, __stdout_used);
weak_alias(dummy_file, __stderr_used);

static void init_file_lock(FILE *f)
{
	if (f && f->lock<0) f->lock = 0;
}

/*
 * ChCore-specific.
 * Since the entry point of the shadow thread should be set to entry directly,
 * we should check whether the new thread is a shadow thread.
 * Besides, creating a shadow thread require its cap as return value,
 * __pthread_create_internal will return the thread cap rather than 0 on success.
 */
int __pthread_create_internal(int type, pthread_t *restrict res,
			      const pthread_attr_t *restrict attrp,
			      void *(*entry)(void *), void *restrict arg)
{
	int ret, c11 = (attrp == __ATTRP_C11_THREAD);
	size_t size, guard;
	struct pthread *self, *new;
	unsigned char *map = 0, *stack = 0, *tsd = 0, *stack_limit;
#if 0
	unsigned flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND
		| CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS
		| CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID | CLONE_DETACHED;
#endif
	pthread_attr_t attr = { 0 };
	sigset_t set;

	if (!libc.can_do_threads) return ENOSYS;
	self = __pthread_self();
	if (!libc.threaded) {
		for (FILE *f=*__ofl_lock(); f; f=f->next)
			init_file_lock(f);
		__ofl_unlock();
		init_file_lock(__stdin_used);
		init_file_lock(__stdout_used);
		init_file_lock(__stderr_used);
		__syscall(SYS_rt_sigprocmask, SIG_UNBLOCK, SIGPT_SET, 0, _NSIG/8);
		self->tsd = (void **)__pthread_tsd_main;
		__membarrier_init();
		libc.threaded = 1;
	}
	if (attrp && !c11) attr = *attrp;

	__acquire_ptc();
	if (!attrp || c11) {
		attr._a_stacksize = CHCORE_PTHREAD_DEFAULT_STACK_SIZE;
		attr._a_guardsize = CHCORE_PTHREAD_DEFAULT_GUARD_SIZE;
	}

	if (attr._a_stackaddr) {
		size_t need = libc.tls_size + __pthread_tsd_size;
		size = attr._a_stacksize;
		stack = (void *)(attr._a_stackaddr & -16);
		stack_limit = (void *)(attr._a_stackaddr - size);
		/* Use application-provided stack for TLS only when
		 * it does not take more than ~12% or 2k of the
		 * application's stack space. */
		if (need < size/8 && need < 2048) {
			tsd = stack - __pthread_tsd_size;
			stack = tsd - libc.tls_size;
			memset(stack, 0, need);
		} else {
			size = ROUND(need);
		}
		guard = 0;
	} else {
		guard = ROUND(attr._a_guardsize);
		size = guard + ROUND(attr._a_stacksize
			+ libc.tls_size +  __pthread_tsd_size);
	}


	if (!tsd) {
		/* TODO: mprotect */
		/* if (guard) */

		map = __mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
		if (map == MAP_FAILED) goto fail;

		tsd = map + size - __pthread_tsd_size;
		if (!stack) {
			stack = tsd - libc.tls_size;
			stack_limit = map + guard;
		}
	}

	new = __copy_tls(tsd - libc.tls_size);
	new->map_base = map;
	new->map_size = size;
	new->stack = stack;
	new->stack_size = stack - stack_limit;
	new->guard_size = guard;
	new->self = new;
	new->tsd = (void *)tsd;
	new->locale = &libc.global_locale;
	if (attr._a_detach) {
		new->detach_state = DT_DETACHED;
	} else {
		new->detach_state = DT_JOINABLE;
	}
	new->robust_list.head = &new->robust_list.head;
	new->canary = self->canary;
	new->sysinfo = self->sysinfo;

	/* Initialize the system ipc structs */
	new->system_ipc_fsm.conn_cap = 0;
	new->system_ipc_fsm.server_id = FS_MANAGER;

	new->system_ipc_net.conn_cap = 0;
	new->system_ipc_net.server_id = NET_MANAGER;

	new->system_ipc_procmgr.conn_cap = 0;
	new->system_ipc_procmgr.server_id = PROC_MANAGER;

	/* Setup argument structure for the new thread on its stack.
	 * It's safe to access from the caller only until the thread
	 * list is unlocked. */
	stack -= (uintptr_t)stack % sizeof(uintptr_t);
#ifdef CHCORE_ARCH_SPARC
	/*
	 * XXX: There is no architecture-specific thread creation.
	 *      So update the stack here. Native musl uses clone to handle this.
	 */
	stack -= PTHREAD_SPARC_STACK_RESERVE;
#endif
	stack -= sizeof(struct start_args);
	struct start_args *args = (void *)stack;
	args->start_func = entry;
	args->start_arg = arg;
	args->control = attr._a_sched ? 1 : 0;

	/* Application signals (but not the synccall signal) must be
	 * blocked before the thread list lock can be taken, to ensure
	 * that the lock is AS-safe. */
	__block_app_sigs(&set);

	/* Ensure SIGCANCEL is unblocked in new thread. This requires
	 * working with a copy of the set so we can restore the
	 * original mask in the calling thread. */
	memcpy(&args->sig_mask, &set, sizeof args->sig_mask);
	args->sig_mask[(SIGCANCEL-1)/8/sizeof(long)] &=
		~(1UL<<((SIGCANCEL-1)%(8*sizeof(long))));

	__tl_lock();

	if (!libc.threads_minus_1++) libc.need_locks = 1;
#if 0
	/* original implementation */

	ret = __clone((c11 ? start_c11 : start), stack, flags, args, &new->tid, TP_ADJ(new), &__thread_list_lock);

	/* All clone failures translate to EAGAIN. If explicit scheduling
	 * was requested, attempt it before unlocking the thread list so
	 * that the failed thread is never exposed and so that we can
	 * clean up all transient resource usage before returning. */
	if (ret < 0) {
		ret = -EAGAIN;
	} else if (attr._a_sched) {
		ret = __syscall(SYS_sched_setscheduler,
			new->tid, attr._a_policy, &attr._a_prio);
		if (a_swap(&args->control, ret ? 3 : 0)==2)
			__wake(&args->control, 1, 1);
		if (ret)
			__wait(&args->control, 0, 3, 0);
	}
#else
	/* chcore create thread (without clone) */

	{
		struct thread_args _args;

		_args.cap_group_cap = 0; /* SELF_CAP_GROUP */

		/*
		 * According to SystemV x86_64 psABI: The value (%rsp + 8) is always
         * a multiple of 16 when control is transferred to the function entry point.
		 * 
		 * So, if the entry point function of a thread is a C function, we 
		 * should simulate that the function is just being "called", i.e., before it
		 * was called, the stack is 16-bytes aligned. After it was called, the stack
		 * should be 8-bytes aligned.(call would push a return address on stack)
		 */
		stack = (unsigned char *)(((unsigned long)stack & (unsigned long)-16) - 8);

		_args.stack = (unsigned long)stack;
		_args.pc = (type != TYPE_USER ? (unsigned long)entry : (unsigned long)(c11 ? start_c11 : start));
		_args.arg = (type != TYPE_USER ? (unsigned long)arg : (unsigned long)args);
		_args.prio = attr._a_sched? attr._a_prio: CHILD_THREAD_PRIO;
		_args.tls = (unsigned long)TP_ADJ(new);
		_args.type = type;
		_args.clear_child_tid = (int *)&__thread_list_lock;

		ret = new->tid = usys_create_thread((unsigned long)&_args);
	}
#endif


	if (ret >= 0) {
		new->next = self->next;
		new->prev = self;
		new->next->prev = new;
		new->prev->next = new;
	} else {
		if (!--libc.threads_minus_1) libc.need_locks = 0;
		ret = -EAGAIN;
	}
	__tl_unlock();
	__restore_sigs(&set);
	__release_ptc();


	if (ret < 0) {
		if (map) __munmap(map, size);
		return ret;
	}

	*res = new;
	return new->tid;
fail:
	__release_ptc();
	return -EAGAIN;
}

/*
 * Create a thread which will run immediately
 * (by inserting it to ready queue in kernel)
 * 1. `pthread_create` return 0 on success
 * 2. The entry of thread should be set to `start` directly
 */
int __pthread_create(pthread_t *restrict res,
		     const pthread_attr_t *restrict attrp,
		     void *(*entry)(void *),
		     void *restrict arg)
{
	int ret = __pthread_create_internal(TYPE_USER, res, attrp, entry, arg);

	return ret < 0? ret: 0;
}

/* Similar to __pthread_create but the return value is thread cap */
cap_t chcore_pthread_create(pthread_t *restrict res,
			  const pthread_attr_t *restrict attrp,
			  void *(*entry)(void *),
			  void *restrict arg)
{
	return __pthread_create_internal(TYPE_USER, res, attrp, entry, arg);
}

/*
 * Create a new shadow thread which will not run immediately
 * (by not inserting it to ready queue in kernel)
 * 1. `pthread_create_shadow` return cap on success
 * 2. The entry of shadow thread should be set to `entry` directly
 */
cap_t chcore_pthread_create_shadow(pthread_t *restrict res,
				 const pthread_attr_t *restrict attrp,
				 void *(*entry)(void *),
				 void *restrict arg)
{
	return __pthread_create_internal(TYPE_SHADOW, res, attrp, entry, arg);
}

/* Separating shadow thread and register cb thread is only for resouce recycling */
cap_t chcore_pthread_create_register_cb(pthread_t *restrict res,
				 const pthread_attr_t *restrict attrp,
				 void *(*entry)(void *),
				 void *restrict arg)
{
	return __pthread_create_internal(TYPE_REGISTER, res, attrp, entry, arg);
}

int chcore_pthread_get_tid(pthread_t thread)
{
	return thread->tid;
}
void chcore_pthread_wake_joiner(pthread_t thread)
{
	libc.threads_minus_1--;
	thread->next->prev = thread->prev;
	thread->prev->next = thread->next;
	thread->prev = thread->next = thread;

	a_store(&thread->detach_state, DT_EXITED);
	__wake(&thread->detach_state, 1, 1);
}


weak_alias(__pthread_exit, pthread_exit);
weak_alias(__pthread_create, pthread_create);
