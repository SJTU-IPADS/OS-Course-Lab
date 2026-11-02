/* Time one read of a register variable and one read of a volatile variable.

     gcc -O0 regmem.c -o regmem && ./regmem */
#include <stdio.h>
#include <time.h>
#include <x86intrin.h>

#define COUNT 5000000L

static volatile long mem = 1;             /* in memory: every read loads from its address */

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void) {
    register long reg = 1;                /* in a CPU register */
    register long sum = 0;

    double start = now();
    for (register long i = 0; i < COUNT; i++)
        sum += reg;
    double reg_time = now() - start;

    /* clflush makes the next read reach memory. This loop times it alone, to subtract below. */
    start = now();
    for (register long i = 0; i < COUNT; i++) {
        _mm_clflush((void *)&mem);
        _mm_mfence();
    }
    double flush_time = now() - start;

    start = now();
    for (register long i = 0; i < COUNT; i++) {
        _mm_clflush((void *)&mem);
        _mm_mfence();
        sum += mem;
    }
    double mem_time = now() - start - flush_time;

    printf("register  %7.2f ns per read\n", reg_time / COUNT * 1e9);
    printf("volatile  %7.2f ns per read\n", mem_time / COUNT * 1e9);
    (void)sum;
    return 0;
}
