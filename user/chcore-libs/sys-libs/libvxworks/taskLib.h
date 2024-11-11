#pragma once
#include <pthread.h>
#include <stdint.h>
#include "vxWorks.h"

STATUS taskDelay
(
 int ticks           /* number of ticks to delay task */
 );

STATUS taskResume
(
 int tid                     /* task ID of task to resume */
 );

typedef struct task_tcb {
	/* Needed by app */
	int status;

	/* Internal status */
	pthread_t pthread_id;
	unsigned int thread_cap;

	FUNCPTR       entryPt;
	int           arg1;
	int           arg2;
	int           arg3;
	int           arg4;
	int           arg5;
	int           arg6;
	int           arg7;
	int           arg8;
	int           arg9;
	int           arg10;
} WIND_TCB;

#define VX_SUPERVISOR_MODE	0x0001
#define VX_UNBREAKABLE		0x0002
#define VX_DEALLOC_STACK	0x0004
#define VX_FP_TASK		0x0008
#define VX_STDIO		0x0010
#define VX_ADA_DEBUG		0x0020
#define VX_FORTRAN		0x0040
#define VX_PRIVATE_ENV		0x0080
#define VX_NO_STACK_FILL	0x0100


int taskSpawn
(
 char          *name,        /* name of new task (stored at pStackBase) */
 int           priority,     /* priority of new task */
 int           options,      /* task option word */
 int           stackSize,    /* size (bytes) of stack needed plus name */
 FUNCPTR       entryPt,      /* entry point of new task */
 int           arg1,         /* 1st of 10 req'd task args to pass to func */
 int           arg2,
 int           arg3,
 int           arg4,
 int           arg5,
 int           arg6,
 int           arg7,
 int           arg8,
 int           arg9,
 int           arg10
 );

STATUS taskSuspend
(
 int tid             /* task ID of task to suspend */
 );

WIND_TCB *taskTcb
(
 int tid                     /* task ID */
 );
