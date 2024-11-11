#define _GNU_SOURCE

#include <pthread.h>
#include <semLib.h>
#include <stdio.h>
#include <time.h>

#include "test_tools.h"

#define SIDE_A_THREAD_NUM       8
#define SIDE_B_THREAD_NUM       16
#define TEST_NUM                1000
#define SEM_COUNT               4

pthread_barrier_t stage_barrier[4];
pthread_mutex_t mutex;
SEM_ID sem[SEM_COUNT];
unsigned volatile waked[SEM_COUNT];
unsigned counter = 0;

void *thread_routine_side_a(void *arg)
{
        unsigned long thread_id = (unsigned long)arg;
        int i, ret;
        struct timespec ts;

        pthread_barrier_wait(&stage_barrier[0]);

        for (i = 0; i < TEST_NUM; i++) {
                ret = semTake(sem[thread_id % SEM_COUNT], WAIT_FOREVER);
                chcore_assert(ret == 0, "semTake failed");
        }

        pthread_mutex_lock(&mutex);
        waked[thread_id % SEM_COUNT]++;
        pthread_mutex_unlock(&mutex);

        pthread_barrier_wait(&stage_barrier[1]);
        if (thread_id == 0) {
                ts.tv_sec = 0;
                ts.tv_nsec = 10 * 1000 * 1000;
                nanosleep(&ts, &ts);
                ret = semTake(sem[0], NO_WAIT);
                chcore_assert(ret == 0, "semTake failed");
        } else if (thread_id == 1) {
                while (semTake(sem[0], NO_WAIT) >= 0);
                semGive(sem[0]);
                ret = semTake(sem[0], NO_WAIT);
                chcore_assert(ret == 0, "semTake failed");
                ret = semTake(sem[0], NO_WAIT);
                chcore_assert(ret < 0, "semTake succeed but should not");
        }

        pthread_barrier_wait(&stage_barrier[2]);
        if (thread_id == 0) {
                ret = semTake(sem[0], 10 * 1000);
                chcore_assert(ret == 0, "semTake failed");
                for (int i = 0; i < SEM_COUNT; i++) {
                        semGive(sem[i]);
                }
        } else if (thread_id == 1) {
                ret = semTake(sem[0], 2 * 1000);
                chcore_assert(ret < 0, "semTake succeed but should not");
        }

        pthread_barrier_wait(&stage_barrier[3]);
        for (i = 0; i < TEST_NUM; i++) {
                while(semTake(sem[0], NO_WAIT) != 0);
                counter++;
                semGive(sem[0]);
        }

        return NULL;
}

void *thread_routine_side_b(void *arg)
{
        unsigned long thread_id = (unsigned long)arg;
        int i;
        struct timespec ts;

        pthread_barrier_wait(&stage_barrier[0]);
        while (waked[thread_id % SEM_COUNT]
                != SIDE_A_THREAD_NUM / SEM_COUNT) {
                semGive(sem[thread_id % SEM_COUNT]);
                sched_yield();
        }

        pthread_barrier_wait(&stage_barrier[1]);
        if (thread_id == 0) {
                ts.tv_sec = 0;
                ts.tv_nsec = 5 * 1000 * 1000;
                nanosleep(&ts, &ts);
                semGive(sem[0]);
        }

        pthread_barrier_wait(&stage_barrier[2]);
        if (thread_id == 0) {
                ts.tv_sec = 0;
                ts.tv_nsec = 5 * 1000 * 1000;
                nanosleep(&ts, &ts);
                semGive(sem[0]);
        }

        pthread_barrier_wait(&stage_barrier[3]);
        for (i = 0; i < TEST_NUM; i++) {
                semTake(sem[0], WAIT_FOREVER);
                counter++;
                semGive(sem[0]);
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        int i, ret;
        pthread_t side_a_thread_id[SIDE_A_THREAD_NUM];
        pthread_t side_b_thread_id[SIDE_B_THREAD_NUM];

        for (i = 0; i < 4; i++) {
                pthread_barrier_init(&stage_barrier[i], NULL,
                        SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM + 1);
        }
        pthread_mutex_init(&mutex, NULL);

        for (i = 0; i < SEM_COUNT; i++) {
                sem[i] = semBCreate(0, SEM_EMPTY);
                waked[i] = 0;
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
        chcore_assert(counter == 
                (SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) * TEST_NUM,
                "incorrect counter");
        info("test stage 4 passed\n");

        for (i = 0; i < SEM_COUNT; i++) {
                ret = semDelete(sem[i]);
                chcore_assert(ret == 0, "semDelete failed");
        }

        info("binary semaphore test finished!\n");

        return 0;
}
