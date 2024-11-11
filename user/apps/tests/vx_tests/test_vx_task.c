#include <pthread.h>
#include <stdio.h>
#include <taskLib.h>

#include "test_tools.h"

#define TASK_NUM        32

pthread_mutex_t mutex;
int resumed_task = 0;

int task_routine(int arg)
{
        info("task %d alive\n", arg);
        taskDelay((arg % 10) * 1000);
        taskSuspend(0);
        info("task %d resumed\n", arg);
        pthread_mutex_lock(&mutex);
        resumed_task++;
        pthread_mutex_unlock(&mutex);

        return 0;
}

int main(int argc, char *argv[])
{
        int task_id[TASK_NUM];
        int i, ret;

        pthread_mutex_init(&mutex, NULL);

        for (i = 0; i < TASK_NUM; i++) {
                task_id[i] = taskSpawn(NULL, i % 4 + 1, 0, 0x200000,
                        task_routine, i, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }

        taskDelay(100 * 1000);
        for (i = 0; i < TASK_NUM; i++) {
                info("Resuming task %d\n", i);
                ret = taskResume(task_id[i]);
                chcore_assert(ret == 0, "taskResume failed");
        }
        taskDelay(100 * 1000);
        chcore_assert(resumed_task == TASK_NUM,
                "some task(s) not resumed");

        info("task test finished!\n");
        return 0;
}
