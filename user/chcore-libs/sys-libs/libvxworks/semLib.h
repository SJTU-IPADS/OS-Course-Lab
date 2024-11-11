#include "vxWorks.h"

typedef enum		/* SEM_B_STATE */
{
	SEM_EMPTY,			/* 0: semaphore not available */
	SEM_FULL			/* 1: semaphore available */
} SEM_B_STATE;

enum {
	SEM_Q_FIFO,
	SEM_Q_PRIORITY
};

typedef struct semaphore /* SEMAPHORE */
{
	/* Only binary semaphore is used */
	int notification_cap;
	volatile char status;
} SEMAPHORE;

typedef struct semaphore *SEM_ID;

#define WAIT_FOREVER (-1)
#define NO_WAIT	     0

SEM_ID semBCreate
(
 int         options,                /* semaphore options */
 SEM_B_STATE initialState            /* initial semaphore state */
);

STATUS semGive
(
 SEM_ID semId        /* semaphore ID to give */
);

STATUS semTake
(
 SEM_ID semId,       /* semaphore ID to take */
 int timeout         /* timeout in ticks */
);

STATUS semDelete
(
 SEM_ID semId        /* semaphore ID to delete */
);
