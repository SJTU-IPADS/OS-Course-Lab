#ifndef TIMER_UTILS_H
#define TIMER_UTILS_H

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

typedef struct {
    const char *name;
    double total_time;
    int calls;
} FunctionTimer;

#define MAX_TIMERS 10
FunctionTimer timers[MAX_TIMERS] = {0};
int num_timers = 0;

double get_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

void start_timer(const char *func_name)
{
    for (int i = 0; i < num_timers; i++) {
        if (strcmp(timers[i].name, func_name) == 0) {
            timers[i].calls++;
            return;
        }
    }
    if (num_timers < MAX_TIMERS) {
        timers[num_timers].name = func_name;
        timers[num_timers].total_time = 0;
        timers[num_timers].calls = 1;
        num_timers++;
    }
}

void end_timer(const char *func_name, double start_time)
{
    double end_time = get_time();
    for (int i = 0; i < num_timers; i++) {
        if (strcmp(timers[i].name, func_name) == 0) {
            timers[i].total_time += end_time - start_time;
            return;
        }
    }
}

void print_timers()
{
    printf("\nPerformance Analysis:\n");
    printf("%-20s %-15s %-15s %-15s\n", "Function", "Total Time(s)", "Calls", "Avg Time(ms)");
    printf("------------------------------------------------------------\n");
    for (int i = 0; i < num_timers; i++) {
        printf("%-20s %-15.6f %-15d %-15.3f\n", timers[i].name, timers[i].total_time, timers[i].calls,
               (timers[i].total_time * 1000.0) / timers[i].calls);
    }
}

#endif // TIMER_UTILS_H
