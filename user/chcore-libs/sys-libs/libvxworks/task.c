#define _GNU_SOURCE
#include <chcore/syscall.h>
#include <chcore/pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "taskLib.h"

/* XXX: maximum number of 64 */
#define MAX_TCB_NUM	64
WIND_TCB *tcbs[MAX_TCB_NUM];
pthread_mutex_t tcb_lock = PTHREAD_MUTEX_INITIALIZER;

STATUS taskDelay(int ticks)
{
        struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = (long)ticks * 1000;
	nanosleep(&ts, NULL);
}

/* TODO: resume threads that are not waiting in taskSuspend */
STATUS taskResume(int tid)
{
	WIND_TCB *tcb;
	int ret;

	tcb = tcbs[tid];
	if (tcb == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = usys_resume(tcb->thread_cap);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return 0;
}

/*
 * XXX: vxworks 5.4 assumes uniprocessor. And threads with the same priority
 * cannot preempt each other. So existing applications' threads are not properly
 * protected with synchronization primitives. Here we set all threads run on
 * core 0 to avoid concurrent execution. Besides, kernel scheduler should
 * guarantee that same-priority thread do not preempt each other.
 */
#define DEFAULT_RUNNING_CPU	0
__thread int cur_tid;
static void *task_wrapper(void *arg)
{
	WIND_TCB *tcb;
	int tid = (int)(long)arg;
        cpu_set_t mask;

	tcb = tcbs[tid];
	cur_tid = tid;

        CPU_ZERO(&mask);
        CPU_SET(DEFAULT_RUNNING_CPU, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask)) {
		printf("%s binding core fails\n", __func__);
		return NULL;
	}

	/* TODO: return value? */
	tcb->entryPt(tcb->arg1, tcb->arg2, tcb->arg3, tcb->arg4,
		     tcb->arg5, tcb->arg6, tcb->arg7, tcb->arg8,
		     tcb->arg9, tcb->arg10);

	return NULL;
}

/* TODO: option parameter */
int taskSpawn(char *name, int priority, int options, int stackSize,
	      FUNCPTR entryPt, int arg1, int arg2, int arg3, int arg4,
	      int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
	int tid, ret;
	pthread_attr_t attr;
	pthread_t thread_id;
	struct sched_param sched_param;
	WIND_TCB *tcb;

	pthread_mutex_lock(&tcb_lock);
	for (tid = 0; tid < MAX_TCB_NUM; tid++) {
		if (tcbs[tid] == NULL) {
			tcbs[tid] = malloc(sizeof(*tcbs[tid]));
			break;
		}
	}
	pthread_mutex_unlock(&tcb_lock);
	if (tid == MAX_TCB_NUM)
		return -ENOBUFS;
	if (!tcbs[tid])
		return -ENOMEM;
	tcb = tcbs[tid];

	tcb->entryPt = entryPt;
	tcb->arg1 = arg1;
	tcb->arg2 = arg2;
	tcb->arg3 = arg3;
	tcb->arg4 = arg4;
	tcb->arg5 = arg5;
	tcb->arg6 = arg6;
	tcb->arg7 = arg7;
	tcb->arg8 = arg8;
	tcb->arg9 = arg9;
	tcb->arg10 = arg10;

	/*
	 * XXX: Temporary implementation of inversing priority since VxWorks
	 *		treats 0 as the highest priority.
	 */
	priority = 255 - priority;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stackSize);
	sched_param.sched_priority = priority;
	pthread_attr_setschedparam(&attr, &sched_param);
	pthread_attr_setinheritsched(&attr, true);

	ret = chcore_pthread_create(&thread_id, &attr, task_wrapper,
		(void *)(long)tid);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	tcb->pthread_id = thread_id;
	tcb->thread_cap = ret;

	return tid;
}

STATUS taskSuspend(int tid)
{
	WIND_TCB *tcb;
	int ret;

	if (tid == 0) {
		ret = usys_suspend(0);
	} else {
		tcb = tcbs[tid];
		if (tcb == NULL) {
			errno = EINVAL;
			return -1;
		}
		ret = usys_suspend(tcb->thread_cap);
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return 0;
}

WIND_TCB *taskTcb(int tid)
{
	if (tid < 0 || tid >= MAX_TCB_NUM)
		return NULL;
	return tcbs[tid];
}
