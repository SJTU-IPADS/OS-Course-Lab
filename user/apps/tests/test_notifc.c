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

#include <assert.h>
#include <chcore/syscall.h>
#include <inttypes.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#include "test_tools.h"

#define notification_debug 0
#define printf_dbg(fmt, args...)             \
        do {                                 \
                if (notification_debug)      \
                        printf(fmt, ##args); \
        } while (0)

#define SIDE_A_THREAD_NUM 8
#define SIDE_B_THREAD_NUM 16

#define BASIC_COUNT 2
/* To ensure total counts of wait and wake are identical */
#define SIDE_A_TEST_COUNT (BASIC_COUNT * SIDE_B_THREAD_NUM)
#define SIDE_B_TEST_COUNT (BASIC_COUNT * SIDE_A_THREAD_NUM)

#define TEST_PHASE_INIT   0
#define TEST_PHASE_NOTIFC 1
#define TEST_PHASE_SEM    2
#define TEST_PHASE_COND   3
volatile int test_phase = TEST_PHASE_INIT;

/* Barriers for sub-tests within a test phase */
volatile int stage1_flag = 0, stage2_flag = 0, stage3_flag = 0;
/* Barriers between test phases */
volatile int test_complate_flag = 0;

/* For testing notifications */
cap_t notifc_cap;
pthread_t side_a_thread_caps[SIDE_A_THREAD_NUM];
pthread_t side_b_thread_caps[SIDE_B_THREAD_NUM];

static int notifc_wait_op(int seletor, int is_block)
{
        return usys_wait(notifc_cap, is_block, NULL);
}

static int notifc_wake_op(int seletor)
{
        return usys_notify(notifc_cap);
}

/* For testing semaphore */
#define SEM_COUNT 4
sem_t sem[SEM_COUNT];

static int sem_wait_op(int seletor, int is_block)
{
        sem_t *cur_sem = &sem[seletor % SEM_COUNT];
        int ret;

        if (is_block) {
                return sem_wait(cur_sem);
        } else {
                ret = sem_trywait(cur_sem);
                /* TODO: errno */
                if (ret != 0)
                        ret = -2;
                return ret;
        }
}

static int sem_wake_op(int seletor)
{
        return sem_post(&sem[seletor % SEM_COUNT]);
}

/* For testing conditional variable */
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cv_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Unlike semaphore and notification whose wake ops are not lost,
 * cond_signal is lost if it is invoked before cond_wait.
 * So use the following notification to guarantee one cond_signal/broadcast
 * is invoked after one cond_wait.
 */
sem_t cond_notifc_sem;

/*
 * pthread_cond does not contain stage 3 test as cond_wait_op cannot be awoken
 * up by previous cond_wake_op.
 */
static bool test_in_stage3(void)
{
        return stage2_flag == SIDE_B_THREAD_NUM;
}

static int cond_wait_op(int seletor, int is_block)
{
        int r = 0;

        if (test_in_stage3())
                return 0;

        pthread_mutex_lock(&cv_lock);
        sem_post(&cond_notifc_sem);
        r = pthread_cond_wait(&cv, &cv_lock);
        pthread_mutex_unlock(&cv_lock);

        return r;
}

static int cond_wake_op(int seletor)
{
        int r = 0;

        if (test_in_stage3())
                return 0;

        sem_wait(&cond_notifc_sem);
        pthread_mutex_lock(&cv_lock);
        if (seletor % 2)
                r = pthread_cond_broadcast(&cv);
        else
                r = pthread_cond_signal(&cv);
        pthread_mutex_unlock(&cv_lock);
        return r;
}

int (*wait_op)(int seletor, int is_block);
int (*wake_op)(int seletor);

/*
 * Helper for ramdon wait/wake
 * Each thread performs: wake - wait - wake - non-blocking wait - ...
 * No random test for pthread_cond, as cond_wake_op and cond_wait_op
 * block each other.
 */
static void stage3_wait_or_wake(int seletor)
{
        int ret;

        if (seletor % 4 == 0 || seletor % 4 == 2) {
                ret = wake_op(0);
                chcore_assert(ret == 0, "stage 3 wake failed");
        } else if (seletor % 4 == 1) {
                /* blocking */
                ret = wait_op(0, true);
                chcore_assert(ret == 0, "stage 3 wait failed");
        } else {
                /* non-blocking */
                do {
                        ret = wait_op(0, false);
                        /*
                         * TODO: the definition of some errno (including EAGAIN
                         * here) in user and kernel is not identical.
                         */
                } while (ret == -2);
        }

        /* somtimes yield */
        if (seletor % 5 == 1 || seletor % 5 == 4)
                sched_yield();
}

/* side_a and side_b wait/wake each other */
void side_a(uint64_t thread_id)
{
        cpu_set_t mask;
        int ret;

        /* binding to different cores */
        CPU_ZERO(&mask);
        CPU_SET(thread_id % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "sched_setaffinity failed!");
        sched_yield();

        /* stage 1: B notifis A. Wake more frequently than wait */
        for (int i = 0; i < SIDE_A_TEST_COUNT; i++) {
                printf_dbg("stage1 thread:%" PRId64 " wait:%d\n", thread_id, i);
                ret = wait_op(i, true);
                chcore_assert(ret == 0, "stage 1 wait failed");
                delay();
        }
        __atomic_add_fetch(&stage1_flag, 1, __ATOMIC_SEQ_CST);
        while (stage1_flag != SIDE_A_THREAD_NUM) {
                usys_yield();
        }

        /* stage 2: A notifis B. Wake less frequently than wait */
        for (int i = 0; i < SIDE_A_TEST_COUNT; i++) {
                printf_dbg("stage2 thread:%" PRId64 " wake:%d\n", thread_id, i);
                ret = wake_op(i);
                chcore_assert(ret == 0, "stage 2 wake failed");
                delay();
        }
        while (stage2_flag != SIDE_B_THREAD_NUM) {
                usys_yield();
        }

        /* stage 3: A and B both wake/wait */
        static_assert(SIDE_A_TEST_COUNT % 2 == 0,
                      "number of threads should be even");
        for (int i = 0; i < SIDE_A_TEST_COUNT; i++) {
                stage3_wait_or_wake(i);
        }
        __atomic_add_fetch(&stage3_flag, 1, __ATOMIC_SEQ_CST);
        while (stage3_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }

        __atomic_add_fetch(&test_complate_flag, 1, __ATOMIC_SEQ_CST);
}

void *thread_routine_side_a(void *arg)
{
        unsigned long thread_id = (unsigned long)arg;

        while (test_phase == TEST_PHASE_INIT) {
                usys_yield();
        }
        side_a(thread_id);

        while (test_phase == TEST_PHASE_NOTIFC) {
                usys_yield();
        }
        side_a(thread_id);

        while (test_phase == TEST_PHASE_SEM) {
                usys_yield();
        }
        side_a(thread_id);

        usys_exit(0);
        return 0;
}

void side_b(uint64_t thread_id)
{
        int ret;

        /* stage 1 */
        for (int i = 0; i < SIDE_B_TEST_COUNT; i++) {
                printf_dbg("stage1 thread:%" PRId64 " wake:%d\n", thread_id, i);
                ret = wake_op(i);
                chcore_assert(ret == 0, "stage 1 wake failed");
        }
        while (stage1_flag != SIDE_A_THREAD_NUM) {
                usys_yield();
        }

        /* stage 2 */
        for (int i = 0; i < SIDE_B_TEST_COUNT; i++) {
                printf_dbg("stage2 thread:%" PRId64 " wait %d\n", thread_id, i);
                ret = wait_op(i, true);
                chcore_assert(ret == 0, "stage 2 wait failed");
        }
        __atomic_add_fetch(&stage2_flag, 1, __ATOMIC_SEQ_CST);
        while (stage2_flag != SIDE_B_THREAD_NUM) {
                usys_yield();
        }

        /* stage 3 */
        static_assert(SIDE_B_TEST_COUNT % 2 == 0,
                      "number of threads should be even");
        for (int i = 0; i < SIDE_B_TEST_COUNT; i++) {
                stage3_wait_or_wake(i);
        }
        __atomic_add_fetch(&stage3_flag, 1, __ATOMIC_SEQ_CST);
        while (stage3_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }

        __atomic_add_fetch(&test_complate_flag, 1, __ATOMIC_SEQ_CST);
}

void *thread_routine_side_b(void *arg)
{
        unsigned long thread_id = (unsigned long)arg;

        while (test_phase == TEST_PHASE_INIT) {
                usys_yield();
        }
        side_b(thread_id);

        while (test_phase == TEST_PHASE_NOTIFC) {
                usys_yield();
        }
        side_b(thread_id);

        while (test_phase == TEST_PHASE_SEM) {
                usys_yield();
        }
        side_b(thread_id);

        usys_exit(0);
        return 0;
}

int main(int argc, char *argv[], char *envp[])
{
        pthread_t thread_id;

        info("Test notific begins...\n");

        for (int i = 0; i < SIDE_A_THREAD_NUM; i++) {
                pthread_create(&thread_id,
                               NULL,
                               thread_routine_side_a,
                               (void *)(unsigned long)i);
                side_a_thread_caps[i] = thread_id;
        }
        for (int i = 0; i < SIDE_B_THREAD_NUM; i++) {
                pthread_create(&thread_id,
                               NULL,
                               thread_routine_side_b,
                               (void *)(unsigned long)i);
                side_b_thread_caps[i] = thread_id;
        }

        /* Test notification */
        wait_op = &notifc_wait_op;
        wake_op = &notifc_wake_op;

        notifc_cap = usys_create_notifc();
        chcore_assert(notifc_cap > 0, "create notification failed!");

        __sync_synchronize();
        test_phase = TEST_PHASE_NOTIFC;

        while (stage1_flag != SIDE_A_THREAD_NUM) {
                usys_yield();
        }
        info("Notifcation test stage 1 finished!\n");

        while (stage2_flag != SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Notifcation test stage 2 finished!\n");

        while (stage3_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Notifcation test stage 3 finished!\n");

        while (test_complate_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Notifcation test finished!\n");

        /* Test semaphore */
        wait_op = &sem_wait_op;
        wake_op = &sem_wake_op;

        for (int i = 0; i < SEM_COUNT; i++)
                sem_init(&sem[i], 0, 0);
        test_complate_flag = 0;
        stage1_flag = 0;
        stage2_flag = 0;
        stage3_flag = 0;

        __sync_synchronize();
        test_phase = TEST_PHASE_SEM;

        while (stage1_flag != SIDE_A_THREAD_NUM) {
                usys_yield();
        }
        info("Semaphore test stage 1 finished!\n");

        while (stage2_flag != SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Semaphore test stage 2 finished!\n");

        while (stage3_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Semaphore test stage 3 finished!\n");

        while (test_complate_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Semaphore test finished!\n");

        /* Test pthread_cond */
        wait_op = &cond_wait_op;
        wake_op = &cond_wake_op;
        sem_init(&cond_notifc_sem, 0, 0);

        test_complate_flag = 0;
        stage1_flag = 0;
        stage2_flag = 0;
        stage3_flag = 0;

        __sync_synchronize();
        test_phase = TEST_PHASE_COND;

        while (stage1_flag != SIDE_A_THREAD_NUM) {
                usys_yield();
        }
        info("Cond test stage 1 finished!\n");

        while (stage2_flag != SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Cond test stage 2 finished!\n");

        while (stage3_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Cond test stage 3 finished!\n");

        while (test_complate_flag != SIDE_A_THREAD_NUM + SIDE_B_THREAD_NUM) {
                usys_yield();
        }
        info("Cond test finished!\n");

        green_info("Test notific finished!\n");

        return 0;
}
