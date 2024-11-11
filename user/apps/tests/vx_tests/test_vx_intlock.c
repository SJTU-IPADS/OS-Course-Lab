#define _GNU_SOURCE

#include <intLib.h>
#include <pthread.h>
#include <sched.h>

#include "test_tools.h"

#define THREAD_NUM      16
#define TEST_NUM        10000

unsigned counter = 0;

void *thread_routine(void *arg)
{
        int i, ret = 0;
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET((long)arg % PLAT_CPU_NUM, &mask);
        ret = sched_setaffinity(0, sizeof(mask), &mask);
        chcore_assert(ret == 0, "Set affinity failed!");

        for (i = 0; i < TEST_NUM; i++) {
                intLock();
                counter++;
                intUnlock(0);
        }

        return NULL;
}

int main(int argc, char *argv[])
{
        pthread_t thread_id[THREAD_NUM];
        int i;

        for (i = 0; i < THREAD_NUM; i++) {
                pthread_create(&thread_id[i],
                                NULL,
                                thread_routine,
                                (void *)(unsigned long)i);
        }

        for (i = 0; i < THREAD_NUM; i++) {
                pthread_join(thread_id[i], NULL);
        }
        chcore_assert(counter == THREAD_NUM * TEST_NUM,
                "incorrect count");

        info("int lock test finished!\n");

        return 0;
}
