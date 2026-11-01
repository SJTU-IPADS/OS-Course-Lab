/* Time one load from a ring of lines placed a fixed distance apart.

   The ring holds `lines` cache lines whose addresses differ by `distance`
   bytes. Each line holds the address of the next one, so the loads cannot
   overlap. Lines 4096 bytes apart fall into the same set of the L1 data cache;
   lines 4160 bytes apart fall into different sets.

     gcc -O2 assoc.c -o assoc && ./assoc */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

static double ns_per_load(int lines, long distance) {
    char *base = aligned_alloc(1 << 21, 64L << 20);
    for (int idx = 0; idx < lines; idx++)
        *(char **)(base + idx * distance) = base + ((idx + 1) % lines) * distance;
    long loads = 1L << 24;
    char *line = base;
    double start = now();
    for (long idx = 0; idx < loads; idx++)
        line = *(char **)line;
    double ns = (now() - start) / loads;
    if (line == NULL) puts("");                       /* keep the loop alive */
    free(base);
    return ns;
}

int main(void) {
    printf("%5s  %16s  %16s\n", "lines", "4160 B apart", "4096 B apart");
    int counts[] = {1, 4, 8, 12, 13, 16, 24};
    for (int step = 0; step < 7; step++)
        printf("%5d  %13.2f ns  %13.2f ns\n", counts[step],
               ns_per_load(counts[step], 4160), ns_per_load(counts[step], 4096));
    return 0;
}
