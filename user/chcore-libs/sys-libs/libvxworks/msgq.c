#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <chcore/syscall.h>

#include "msgQLib.h"
#include "intLib.h"

MSG_Q_ID msgQCreate(int maxMsgs, /* max messages that can be queued */
                    int maxMsgLength, /* max bytes in a message */
                    int options /* message queue options */)
{
        if ((options != MSG_Q_FIFO) && (options != MSG_Q_PRIORITY)) {
                printf("%s: invalid msgq option %d\n", __func__, options);
                return NULL;
        }

        MSG_Q_ID msgq = (MSG_Q_ID)malloc(sizeof(*msgq));
        if (msgq == NULL) {
                return NULL;
        }
        init_list_head(&(msgq->list));
        init_list_head(&(msgq->list_high_prio));
        msgq->not_empty_notifc = usys_create_notifc();
        msgq->not_full_notifc = usys_create_notifc();
        msgq->maxMsgs = maxMsgs;
        msgq->maxMsgLength = maxMsgLength;
        msgq->options = options;
        msgq->current_msg_count = 0;
        msgq->current_slot_count = maxMsgs;

        return msgq;
}

/* TODO: App needs to ensure no contention between msgQDelete and other ops. */
STATUS msgQDelete(MSG_Q_ID msgQId /* message queue to delete */)
{
        struct msg_template *msg_iter, *msg_tmp;
        /* Free each msg one by one */
        for_each_in_list_safe (msg_iter, msg_tmp, node, &(msgQId->list)) {
                list_del(&(msg_iter->node));
                free(msg_iter);
        };

        if(msgQId->options == MSG_Q_PRIORITY) {
                /* Free each high-priority msg one by one */
                for_each_in_list_safe (msg_iter, msg_tmp, node, &(msgQId->list)) {
                        list_del(&(msg_iter->node));
                        free(msg_iter);
                };
        }
        
        free(msgQId);

        // TODO: unblock all the pending threads

        return 0;
}

int msgQNumMsgs(FAST MSG_Q_ID msgQId /* message queue to examine */)
{
        return msgQId->current_msg_count;
}

static uint32_t retrieve_one_msg_from_list(struct list_head *head,
                char *buffer, uint32_t max_bytes)
{
        struct msg_template *msg = NULL;
        uint32_t res = 0;


        if (!list_empty(head)) {
                msg = list_entry(head->next, struct msg_template, node);
                list_del(&(msg->node));
        }

        if (msg) {
                res = (msg->msg_len < max_bytes) ? msg->msg_len : max_bytes;
                memcpy(buffer, &(msg->payload), res);
                free(msg);
        }

        return res;
}

int msgQReceive(FAST MSG_Q_ID msgQId, /* message queue from which to receive */
                char *buffer, /* buffer to receive message */
                uint32_t maxNBytes, /* length of buffer */
                int timeout /* ticks to wait */)
{
        int ret = -1;
        struct timespec ts;

        intLock();

        switch (timeout) {
        case NO_WAIT:
                if (msgQId->current_msg_count > 0) {
                        msgQId->current_msg_count--;
                        ret = 0;
                }
                break;
        case WAIT_FOREVER:
                while (msgQId->current_msg_count <= 0) {
                        intUnlock(0);
                        usys_wait(msgQId->not_empty_notifc, true, NULL);
                        intLock();
                }
                msgQId->current_msg_count--;
                ret = 0;
                break;
        default:
                ts.tv_sec = timeout / 1000000L;
                ts.tv_nsec = (timeout % 1000000L) * 1000L;
                ret = 0;

                while (msgQId->current_msg_count <= 0) {
                        intUnlock(0);
                        ret = usys_wait(msgQId->not_empty_notifc, true, &ts);
                        intLock();
                        if (ret == -ETIMEDOUT) {
                                ret = -1;
                                break;
                        } 
                }
                if (ret == 0) {
                        msgQId->current_msg_count--;
                }
                break;
        }

        if (ret == 0) {
                if (msgQId->options == MSG_Q_PRIORITY) {
                        ret = retrieve_one_msg_from_list(
                                &msgQId->list_high_prio,
                                buffer, maxNBytes);
                }
                if (ret == 0) {
                        ret = retrieve_one_msg_from_list(
                                &(msgQId->list),
                                buffer, maxNBytes);
                }
                if (ret == 0) {
                        printf("count = %d\n", msgQId->current_msg_count);
                }
        }

        if (ret > 0) {
                msgQId->current_slot_count++;
                usys_notify(msgQId->not_full_notifc);
        }
        intUnlock(0);

        return ret;
}

static void add_one_msg_into_list(struct list_head *head,
                char *buffer, uint32_t bytes)
{
        struct msg_template *msg = NULL;

        msg = (struct msg_template *)malloc(bytes + sizeof(*msg));
        msg->msg_len = bytes;
        memcpy(&(msg->payload), buffer, bytes);
        list_append(&(msg->node), head);
}

STATUS msgQSend(FAST MSG_Q_ID msgQId, /* message queue on which to send */
                char *buffer, /* message to send */
                FAST uint32_t nBytes, /* length of message */
                int timeout, /* ticks to wait */
                int priority /* MSG_PRI_NORMAL or MSG_PRI_URGENT */)
{
        int ret = -1;
        struct timespec ts;

        if (nBytes > msgQId->maxMsgLength) {
                return -1;
        }

        if ((priority != MSG_PRI_NORMAL) && (priority != MSG_PRI_URGENT)) {
                return -1;
        }

        intLock();

        switch (timeout) {
        case NO_WAIT:
                if (msgQId->current_slot_count > 0) {
                        msgQId->current_slot_count--;
                        ret = 0;
                }
                break;
        case WAIT_FOREVER:
                while (msgQId->current_msg_count >= msgQId->maxMsgs) {
                        intUnlock(0);
                        usys_wait(msgQId->not_full_notifc, true, NULL);
                        intLock();
                }
                msgQId->current_slot_count--;
                ret = 0;
                break;
        default:
                ts.tv_sec = timeout / 1000000L;
                ts.tv_nsec = (timeout % 1000000L) * 1000L;

                while (msgQId->current_msg_count >= msgQId->maxMsgs) {
                        intUnlock(0);
                        ret = usys_wait(msgQId->not_full_notifc, true, &ts);
                        intLock();
                        if (ret == -ETIMEDOUT) {
                                ret = -1;
                                break;
                        } else {
                                ret = 0;
                        }
                }
                if (ret == 0) {
                        msgQId->current_slot_count--;
                }
                break;
        }

        if (ret == 0) {
                if (priority == MSG_PRI_NORMAL) {
                        add_one_msg_into_list(&(msgQId->list),
                                buffer, nBytes);
                } else {
                        add_one_msg_into_list(&(msgQId->list_high_prio),
                                buffer, nBytes);
                }

                msgQId->current_msg_count++;
                usys_notify(msgQId->not_empty_notifc);
        }
        intUnlock(0);

        return ret;
}
