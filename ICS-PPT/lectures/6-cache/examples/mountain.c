/* mountain.c - read throughput as a function of working set size and stride.

   The program reads a prefix of an array of long ("working set") with a given
   stride and reports how many GB/s it read, for sizes from 128 MB down to 8 KB
   and strides from 1 up to a limit (16 elements, or the first argument).

     gcc -O2 mountain.c -o mountain && ./mountain 8 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MINBYTES (1 << 13)               /* working set sizes: 8 KB ...   */
#define MAXBYTES (1 << 27)               /* ... up to 128 MB              */
#define MAXSTRIDE 16                     /* default: strides 1 to 16      */
#define MAXELEMS (MAXBYTES / sizeof(long))

long data[MAXELEMS];                     /* the array that is traversed   */

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* Read every stride-th element of data[0 .. elems-1] with four independent sums. */
long test(long elems, long stride) {
    long sx2 = stride * 2, sx3 = stride * 3, sx4 = stride * 4;
    long acc0 = 0, acc1 = 0, acc2 = 0, acc3 = 0;
    long limit = elems - sx4;
    long i;
    for (i = 0; i < limit; i += sx4) {
        acc0 += data[i];
        acc1 += data[i + stride];
        acc2 += data[i + sx2];
        acc3 += data[i + sx3];
    }
    for (; i < elems; i += stride)       /* the remaining elements        */
        acc0 += data[i];
    return (acc0 + acc1) + (acc2 + acc3);
}

/* Read throughput in GB/s of test(elems, stride). */
double run(long size, long stride) {
    long elems = size / sizeof(long);
    volatile long sink;
    double best = 0;
    sink = test(elems, stride);          /* warm up the cache             */
    for (int rep = 0; rep < 5; rep++) {
        long calls = 0;
        double start = now(), elapsed;
        do {
            sink = test(elems, stride);
            calls++;
            elapsed = now() - start;
        } while (elapsed < 0.02);
        double gbps = (double)calls * (elems / stride) * sizeof(long) / elapsed / 1e9;
        if (gbps > best) best = gbps;
    }
    (void)sink;
    return best;
}

int main(int argc, char **argv) {
    long maxstride = argc > 1 ? atol(argv[1]) : MAXSTRIDE;
    for (long i = 0; i < MAXELEMS; i++) data[i] = 1;
    printf("%8s", "size");
    for (long stride = 1; stride <= maxstride; stride++) printf("  s%-4ld", stride);
    printf("\n");
    for (long size = MAXBYTES; size >= MINBYTES; size >>= 1) {
        if (size >= 1 << 20) printf("%6ld M", size >> 20);
        else printf("%6ld K", size >> 10);
        for (long stride = 1; stride <= maxstride; stride++)
            printf("  %5.1f", run(size, stride));
        printf("\n");
    }
    return 0;
}
