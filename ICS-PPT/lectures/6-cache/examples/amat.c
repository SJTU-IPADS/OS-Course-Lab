/* Average time of one load as a function of the hit rate.

   Every load reads one 64-byte line. With probability `rate` the line comes
   from a 16 KB hot set that stays in the L1 cache; otherwise it comes from a
   256 MB cold set that is never in a cache. The address of each load depends
   on the value of the previous load, so the loads do not overlap.

     gcc -O2 amat.c -o amat && ./amat */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LINE 64
#define HOT (16L << 10)
#define COLD (256L << 20)
#define LOADS (1L << 24)

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(int argc, char **argv) {
    long zero = argc - 1;                             /* 0, but unknown to the compiler */
    char *hot = aligned_alloc(1 << 21, HOT);
    char *cold = aligned_alloc(1 << 21, COLD);
    for (long idx = 0; idx < HOT; idx++) hot[idx] = 0;
    for (long idx = 0; idx < COLD; idx++) cold[idx] = 0;
    char **addr = malloc(LOADS * sizeof *addr);
    printf("%8s  %12s\n", "hit rate", "ns per load");
    double rates[] = {0.0, 0.5, 0.9, 0.97, 0.99, 1.0};
    for (int step = 0; step < 6; step++) {
        srand(1);
        for (long idx = 0; idx < LOADS; idx++) {
            if (rand() < rates[step] * RAND_MAX)
                addr[idx] = hot + (long)(rand() % (HOT / LINE)) * LINE;
            else
                addr[idx] = cold + (((long)rand() << 16) ^ rand()) % (COLD / LINE) * LINE;
        }
        long sum = 0;
        double start = now();
        for (long idx = 0; idx < LOADS; idx++)
            sum = *(addr[idx] + sum) & zero;          /* the address depends on the previous load */
        double ns = (now() - start) / LOADS;
        printf("%7.0f%%  %12.1f\n", rates[step] * 100, ns + sum);
    }
    return 0;
}
