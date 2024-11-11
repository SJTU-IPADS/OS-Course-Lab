#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chcore/syscall.h>

#include "semLib.h"
#include "intLib.h"

SEM_ID semBCreate(int options, SEM_B_STATE initialState)
{
	SEM_ID sem;

	if (options != SEM_Q_FIFO)
		return NULL;

	sem = malloc(sizeof(*sem));
	if (!sem)
		return NULL;

	sem->status = initialState;
	sem->notification_cap = usys_create_notifc();
	if (sem->notification_cap < 0)
		return NULL;

	return sem;
}

/* XXX: Concurrent semGive is not handled. May send notification for multiple times. */
STATUS semGive(SEM_ID semId)
{
	int old_status;
	old_status = semId->status;
	semId->status = SEM_EMPTY;
	if (old_status == SEM_FULL)
		usys_notify(semId->notification_cap);
	return 0;
}

STATUS semTake(SEM_ID semId, int timeout)
{
	int ret = -1;
	struct timespec ts;

	intLock();

	switch (timeout) {
	case NO_WAIT:
		if (semId->status == SEM_EMPTY) {
			semId->status = SEM_FULL;
			ret = 0;
		}
		break;
	case WAIT_FOREVER:
		while (semId->status == SEM_FULL) {
			intUnlock(0);
			usys_wait(semId->notification_cap, true, NULL);
			intLock();
		}
		semId->status = SEM_FULL;
		ret = 0;
		break;
	default:
		ts.tv_sec = timeout / 1000000L;
		ts.tv_nsec = (timeout % 1000000L) * 1000L;

		while (semId->status == SEM_FULL) {
			intUnlock(0);
			ret = usys_wait(semId->notification_cap, true, &ts);
			intLock();
			if (ret == -ETIMEDOUT) {
				ret = -1;
				break;
			} else {
				ret = 0;
			}
		}
		if (ret == 0) {
			semId->status = SEM_FULL;
		}
		break;
	}

	intUnlock(0);

	return ret;
}

STATUS semDelete(SEM_ID semId)
{
	/* XXX: not waking up pending tasks */
	free(semId);
	return 0;
}
