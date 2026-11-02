/* Time two dependent loads whose addresses are `offset` bytes apart.

   The data is a ring of 256-byte nodes in random order. The first load of a
   node reads the address of its second load, `offset` bytes further on; the
   second load reads the address of the next node. Both loads of a node are
   served by one cache line when the offsets are in the same line, and by two
   lines otherwise.

     gcc -O2 linesize.c -o linesize && ./linesize */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>

#define NODE 256
#define BYTES (256L << 20)

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(void) {
    long count = BYTES / NODE;
    char *base = aligned_alloc(1 << 21, BYTES);
    madvise(base, BYTES, MADV_HUGEPAGE);
    long *order = malloc(count * sizeof *order);
    for (long idx = 0; idx < count; idx++) order[idx] = idx;
    srand(1);
    for (long idx = count - 1; idx > 0; idx--) {
        long other = rand() % idx;
        long tmp = order[idx]; order[idx] = order[other]; order[other] = tmp;
    }
    printf("%6s  %12s\n", "offset", "ns per node");
    long offsets[] = {8, 32, 56, 64, 128, 192};
    for (int step = 0; step < 6; step++) {
        long offset = offsets[step];
        for (long idx = 0; idx < count; idx++) {
            char *node = base + order[idx] * NODE;
            *(char **)node = node + offset;
            *(char **)(node + offset) = base + order[(idx + 1) % count] * NODE;
        }
        char *ptr = base + order[0] * NODE;
        double start = now();
        for (long idx = 0; idx < count; idx++) {
            ptr = *(char **)ptr;
            ptr = *(char **)ptr;
        }
        double ns = (now() - start) / count;
        if (ptr == NULL) puts("");                    /* keep the loop alive */
        printf("%6ld  %12.1f\n", offset, ns);
    }
    return 0;
}
