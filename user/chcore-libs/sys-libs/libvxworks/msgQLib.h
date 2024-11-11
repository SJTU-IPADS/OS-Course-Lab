#pragma once

#include <chcore/container/list.h>
#include "vxWorks.h"

#if 0 /* VxWorks defs */
typedef struct obj_core /* OBJ_CORE */ {
        int valid; /* validation field */
} OBJ_CORE; /* object core */

typedef struct msg_q *MSG_Q_ID; /* message queue ID */

typedef struct msg_q /* MSG_Q */ {
        OBJ_CORE objCore; /* object management */
        Q_JOB_HEAD msgQ; /* message queue head */
        Q_JOB_HEAD freeQ; /* free message queue head */
        int options; /* message queue options */
        int maxMsgs; /* max number of messages in queue */
        int maxMsgLength; /* max length of message */
        int sendTimeouts; /* number of send timeouts */
        int recvTimeouts; /* number of receive timeouts */
} MSG_Q;

typedef struct /* Head of job queue */ {
        Q_JOB_NODE *first; /* first node in queue */
        Q_JOB_NODE *last; /* last node in queue */
        int count; /* number of nodes in queue */
        Q_CLASS *pQClass; /* must be 4th long word */
        Q_HEAD pendQ; /* queue of blocked tasks */
} Q_JOB_HEAD;
#endif

typedef struct msg_q *MSG_Q_ID; /* message queue ID */
 
#define MSG_Q_FIFO      (0x00)
#define MSG_Q_PRIORITY  (0x01)

struct msg_q /* MSG_Q */ {
        struct list_head list;
        struct list_head list_high_prio;
        int not_empty_notifc;
        int not_full_notifc;
        int options; /* message queue options */
        int maxMsgs; /* max number of messages in queue */
        int maxMsgLength; /* max length of message */
        int current_msg_count;
        int current_slot_count;
        // int sendTimeouts; /* number of send timeouts */
        // int recvTimeouts; /* number of receive timeouts */
};


struct msg_template {
        struct list_head node;
        uint32_t msg_len;
        char payload[0];
};

/*******************************************************************************
 * *
 * * msgQCreate - create and initialize a message queue
 * *
 * * This routine creates a message queue capable of holding up to <maxMsgs>
 * * messages, each up to <maxMsgLength> bytes long.  The routine returns
 * * a message queue ID used to identify the created message queue in all
 * * subsequent calls to routines in this library.  The queue can be created
 * * with the following options:
 * * .iP "MSG_Q_FIFO  (0x00)" 8
 * * queue pended tasks in FIFO order.
 * * .iP "MSG_Q_PRIORITY  (0x01)"
 * * queue pended tasks in priority order.
 * *
 * * RETURNS:
 * * MSG_Q_ID, or NULL if error.
 * *
 * * ERRNO: S_memLib_NOT_ENOUGH_MEMORY, S_intLib_NOT_ISR_CALLABLE
 * *
 * * SEE ALSO: msgQSmLib
 * */

MSG_Q_ID msgQCreate(int maxMsgs, /* max messages that can be queued */
                    int maxMsgLength, /* max bytes in a message */
                    int options /* message queue options */);

/*******************************************************************************
 * *
 * * msgQDelete - delete a message queue
 * *
 * * This routine deletes a message queue.  Any task blocked on either
 * * a msgQSend() or msgQReceive() will be unblocked and receive an error
 * * from the call with `errno' set to S_objLib_OBJECT_DELETED.  The
 * * <msgQId> parameter will no longer be a valid message queue ID.
 * *
 * * RETURNS: OK or ERROR.
 * *
 * * ERRNO: S_objLib_OBJ_ID_ERROR, S_intLib_NOT_ISR_CALLABLE
 * *
 * * SEE ALSO: msgQSmLib
 * */

STATUS msgQDelete(MSG_Q_ID msgQId /* message queue to delete */);

/*******************************************************************************
 * *
 * * msgQNumMsgs - get the number of messages queued to a message queue
 * *
 * * This routine returns the number of messages currently queued to a specified
 * * message queue.
 * *
 * * RETURNS:
 * * The number of messages queued, or ERROR.
 * *
 * * ERRNO:  S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED,
 * *         S_objLib_OBJ_ID_ERROR
 * *
 * * SEE ALSO: msgQSmLib
 * */

int msgQNumMsgs(FAST MSG_Q_ID msgQId /* message queue to examine */);

/*******************************************************************************
 * *
 * * msgQReceive - receive a message from a message queue
 * *
 * * This routine receives a message from the message queue <msgQId>.
 * * The received message is copied into the specified <buffer>, which is
 * * <maxNBytes> in length.  If the message is longer than <maxNBytes>,
 * * the remainder of the message is discarded (no error indication
 * * is returned).
 * *
 * * The <timeout> parameter specifies the number of ticks to wait for
 * * a message to be sent to the queue, if no message is available when
 * * msgQReceive() is called.  The <timeout> parameter can also have
 * * the following special values:
 * * .iP "NO_WAIT  (0)" 8
 * * return immediately, even if the message has not been sent.
 * * .iP "WAIT_FOREVER  (-1)"
 * * never time out.
 * * .LP
 * *
 * * WARNING: This routine must not be called by interrupt service routines.
 * *
 * * RETURNS:
 * * The number of bytes copied to <buffer>, or ERROR.
 * *
 * * ERRNO: S_distLib_NOT_INITIALIZED, S_smObjLib_NOT_INITIALIZED,
 * *        S_objLib_OBJ_ID_ERROR, S_objLib_OBJ_DELETED,
 * *        S_objLib_OBJ_UNAVAILABLE, S_objLib_OBJ_TIMEOUT,
 * *        S_msgQLib_INVALID_MSG_LENGTH
 * *
 * * SEE ALSO: msgQSmLib
 * */

#define NO_WAIT  (0)
#define WAIT_FOREVER  (-1)

int msgQReceive(FAST MSG_Q_ID msgQId, /* message queue from which to receive */
                char *buffer, /* buffer to receive message */
                uint32_t maxNBytes, /* length of buffer */
                int timeout /* ticks to wait */);

/*******************************************************************************
 * *
 * * msgQSend - send a message to a message queue
 * *
 * * This routine sends the message in <buffer> of length <nBytes> to the
 * message
 * * queue <msgQId>.  If any tasks are already waiting to receive messages
 * * on the queue, the message will immediately be delivered to the first
 * * waiting task.  If no task is waiting to receive messages, the message
 * * is saved in the message queue.
 * *
 * * The <timeout> parameter specifies the number of ticks to wait for free
 * * space if the message queue is full.  The <timeout> parameter can also have
 * * the following special values:
 * * .iP "NO_WAIT  (0)" 8
 * * return immediately, even if the message has not been sent.
 * * .iP "WAIT_FOREVER  (-1)"
 * * never time out.
 * * .LP
 * *
 * * The <priority> parameter specifies the priority of the message being sent.
 * * The possible values are:
 * * .iP "MSG_PRI_NORMAL  (0)" 8
 * * normal priority; add the message to the tail of the list of queued
 * * messages.
 * * .iP "MSG_PRI_URGENT  (1)"
 * * urgent priority; add the message to the head of the list of queued
 * messages.
 * * .LP
 * *
 * * USE BY INTERRUPT SERVICE ROUTINES
 * * This routine can be called by interrupt service routines as well as
 * * by tasks.  This is one of the primary means of communication
 * * between an interrupt service routine and a task.  When called from an
 * * interrupt service routine, <timeout> must be NO_WAIT.
 * *
 * * RETURNS: OK or ERROR.
 * *
 * * ERRNO:  S_distLib_NOT_INITIALIZED, S_objLib_OBJ_ID_ERROR,
 * *         S_objLib_OBJ_DELETED, S_objLib_OBJ_UNAVAILABLE,
 * *         S_objLib_OBJ_TIMEOUT, S_msgQLib_INVALID_MSG_LENGTH,
 * *         S_msgQLib_NON_ZERO_TIMEOUT_AT_INT_LEVEL
 * *
 * * SEE ALSO: msgQSmLib
 * */

#define MSG_PRI_NORMAL  (0)
#define MSG_PRI_URGENT  (1)

STATUS msgQSend(FAST MSG_Q_ID msgQId, /* message queue on which to send */
                char *buffer, /* message to send */
                FAST uint32_t nBytes, /* length of message */
                int timeout, /* ticks to wait */
                int priority /* MSG_PRI_NORMAL or MSG_PRI_URGENT */);
