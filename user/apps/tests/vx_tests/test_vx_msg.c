#define _GNU_SOURCE

#include <msgQLib.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#include "test_tools.h"

#define SIDE_A_THREAD_NUM       8
#define SIDE_B_THREAD_NUM       16
/* To ensure total counts of send and receive are identical */
#define BASIC_COUNT             1000
#define SIDE_A_TEST_COUNT       (BASIC_COUNT * SIDE_B_THREAD_NUM)
#define SIDE_B_TEST_COUNT       (BASIC_COUNT * SIDE_A_THREAD_NUM)

#define MAX_MSG_NUM             (SIDE_A_THREAD_NUM * SIDE_A_TEST_COUNT)
#define MAX_MSG_LEN             128
#define TEST_QUEUE_NUM          2

pthread_barrier_t stage_barrier[4];
MSG_Q_ID msg_queue[TEST_QUEUE_NUM];

static void construct_msg(char *buf, unsigned length)
{
        int i;
        unsigned *buf_start = (unsigned *)buf;
        *buf_start = length;
        for (i = sizeof(unsigned); i < length; i++) {
                buf[i] = 'a' + (length + i) % 26;
        }
}

static void verify_msg(char *buf, unsigned length)
{
        int i;
        unsigned *buf_start = (unsigned *)buf;
        chcore_assert(length >= 4, "msg length shorter than expected");
        chcore_assert(length == *buf_start, "msg length not match");
        for (i = sizeof(unsigned); i < length; i++) {
                chcore_assert(buf[i] == ('a' + (length + i) % 26),
                        "msg corrupted");
        }
}

void *thread_routine_side_a(void *arg)
{
        int i, j, ret;
        char buf[MAX_MSG_LEN];
        unsigned length;
        long sleep_ms;
        struct timespec ts;

        sleep_ms = (long)arg % 5;
        ts.tv_sec = 0;
        ts.tv_nsec = sleep_ms * 1000 * 1000;

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                for (j = 0; j < SIDE_A_TEST_COUNT; j++) {
                        length = rand() % (MAX_MSG_LEN - 4) + 4;
                        construct_msg(buf, length);
                        ret = msgQSend(msg_queue[i], buf, length,
                                NO_WAIT, MSG_PRI_NORMAL);
                        chcore_assert(ret == 0, "msgQSend failed");
                }
        }

        pthread_barrier_wait(&stage_barrier[0]);
        pthread_barrier_wait(&stage_barrier[1]);
        pthread_barrier_wait(&stage_barrier[2]);

        nanosleep(&ts, &ts);
        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                for (j = 0; j < 2 * SIDE_A_TEST_COUNT; j++) {
                        length = rand() % (MAX_MSG_LEN - 4) + 4;
                        construct_msg(buf, length);
                        ret = msgQSend(msg_queue[i], buf, length,
                                WAIT_FOREVER, MSG_PRI_NORMAL);
                        chcore_assert(ret == 0, "msgQSend failed");
                }
        }

        pthread_barrier_wait(&stage_barrier[3]);

        if ((long)arg == 0) {
                ts.tv_sec = 0;
                ts.tv_nsec = 10 * 1000 * 1000;
                nanosleep(&ts, &ts);
                length = rand() % (MAX_MSG_LEN - 4) + 4;
                construct_msg(buf, length);
                ret = msgQSend(msg_queue[0], buf, length,
                        WAIT_FOREVER, MSG_PRI_NORMAL);
                chcore_assert(ret == 0, "msgQSend failed");
        }

        return NULL;
}

void *thread_routine_side_b(void *arg)
{
        int i, j, ret;
        char buf[MAX_MSG_LEN];

        pthread_barrier_wait(&stage_barrier[0]);

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                for (j = 0; j < SIDE_B_TEST_COUNT; j++) {
                        ret = msgQNumMsgs(msg_queue[i]);
                        chcore_assert(ret != 0, "msg queue is empty");
                        ret = msgQReceive(msg_queue[i], buf,
                                MAX_MSG_LEN, NO_WAIT);
                        verify_msg(buf, ret);
                }
        }

        pthread_barrier_wait(&stage_barrier[1]);

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                ret = msgQNumMsgs(msg_queue[i]);
                chcore_assert(ret == 0, "msg queue is not empty");
                ret = msgQReceive(msg_queue[i], buf,
                                MAX_MSG_LEN, NO_WAIT);
                chcore_assert(ret <= 0, "msg queue is not empty");
        }

        pthread_barrier_wait(&stage_barrier[2]);

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                for (j = 0; j < 2 * SIDE_B_TEST_COUNT; j++) {
                        ret = msgQReceive(msg_queue[i], buf,
                                MAX_MSG_LEN, WAIT_FOREVER);
                        verify_msg(buf, ret);
                }
        }

        pthread_barrier_wait(&stage_barrier[3]);

        if ((long)arg == 0) {
                ret = msgQReceive(msg_queue[0], buf,
                        MAX_MSG_LEN, 15 * 1000);
                verify_msg(buf, ret);
        } else if ((long)arg == 1){
                ret = msgQReceive(msg_queue[0], buf,
                        MAX_MSG_LEN, 5 * 1000);
                chcore_assert(ret < 0, "not timed out");
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        pthread_t side_a_thread_id[SIDE_A_THREAD_NUM];
        pthread_t side_b_thread_id[SIDE_B_THREAD_NUM];
        int i = 0, ret;

        for (i = 0; i < 4; i++) {
                pthread_barrier_init(&stage_barrier[i], NULL,
                        SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM + 1);
        }

        srand(time(NULL));

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                msg_queue[i] = msgQCreate(MAX_MSG_NUM, MAX_MSG_LEN, 0);
        }

        for (i = 0; i < SIDE_A_THREAD_NUM; i++) {
                pthread_create(&side_a_thread_id[i],
                                NULL,
                                thread_routine_side_a,
                                (void *)(unsigned long)i);
        }
        for (i = 0; i < SIDE_B_THREAD_NUM; i++) {
                pthread_create(&side_b_thread_id[i],
                                NULL,
                                thread_routine_side_b,
                                (void *)(unsigned long)i);
        }

        pthread_barrier_wait(&stage_barrier[0]);
        pthread_barrier_wait(&stage_barrier[1]);
        info("test stage 1 passed\n");
        pthread_barrier_wait(&stage_barrier[2]);
        info("test stage 2 passed\n");
        pthread_barrier_wait(&stage_barrier[3]);
        info("test stage 3 passed\n");

        for (i = 0; i < SIDE_A_THREAD_NUM; i++) {
                pthread_join(side_a_thread_id[i], NULL);
        }
        for (i = 0; i < SIDE_B_THREAD_NUM; i++) {
                pthread_join(side_b_thread_id[i], NULL);
        }
        info("test stage 4 passed\n");

        for (i = 0; i < TEST_QUEUE_NUM; i++) {
                ret = msgQDelete(msg_queue[i]);
                chcore_assert(ret == 0, "msgQDelete failed");
        }

        info("message queue test finished!\n");
        return 0;
}
