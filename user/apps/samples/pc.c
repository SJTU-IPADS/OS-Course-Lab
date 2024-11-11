#define _GNU_SOURCE
#include <pthread.h>

void *b(void *arg)
{
        return 0;
}

void *a(void *arg)
{
        pthread_t thread;
        pthread_attr_t pthread_attr;
        struct sched_param sched_param;

        sched_param.sched_priority = 2;
        pthread_attr_init(&pthread_attr);
        pthread_attr_setinheritsched(&pthread_attr, 1);
        pthread_attr_setschedparam(&pthread_attr, &sched_param);
        pthread_create(&thread, &pthread_attr, b, NULL);
        pthread_join(thread, NULL);
        
        return 0;
}

int main(int argc, char *argv[])
{
        pthread_t thread;
        pthread_attr_t pthread_attr;
        struct sched_param sched_param;

        sched_param.sched_priority = 2;
        pthread_attr_init(&pthread_attr);
        pthread_attr_setinheritsched(&pthread_attr, 1);
        pthread_attr_setschedparam(&pthread_attr, &sched_param);
        pthread_create(&thread, &pthread_attr, a, NULL);
        pthread_join(thread, NULL);
        
        return 0;
}