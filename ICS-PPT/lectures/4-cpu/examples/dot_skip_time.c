/* Time of dot_skip on the input named on the command line:

     ./dot_skip none        no element of w is 0
     ./dot_skip front       the first half of w is 0
     ./dot_skip random      half of w is 0, at random positions
     ./dot_skip alternate   every other element of w is 0

   front, random and alternate hold the same values of w in different
   orders, so dot_skip executes the same instructions on each of them; what
   differs is the order in which its if jumps one way or the other.

   dot_skip.c is compiled with -Og and this file with -O2 (see Makefile).
   dot_skip is in a file of its own, so the call below is not inlined.
   The time printed is that of the fastest of PASSES calls, per element. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int dot_skip(int *w, int *x, int n);

#define N (1 << 16)                     /* elements of w and x */
#define PASSES 2000                     /* calls of dot_skip */
static int w[N], x[N];
volatile int sink;

int main(int argc, char **argv) {
    const char *input = argc > 1 ? argv[1] : "";
    srand(1);
    for (int i = 0; i < N; i++) {
        w[i] = rand() % 127 + 1;        /* 1 .. 127 */
        x[i] = rand() % 256 - 128;
    }
    if (strcmp(input, "none") == 0) {
    } else if (strcmp(input, "front") == 0) {
        for (int i = 0; i < N / 2; i++)
            w[i] = 0;
    } else if (strcmp(input, "random") == 0) {
        for (int i = 0; i < N / 2; i++)
            w[i] = 0;
        for (int i = N - 1; i > 0; i--) {   /* shuffle */
            int j = rand() % (i + 1), t = w[i];
            w[i] = w[j];
            w[j] = t;
        }
    } else if (strcmp(input, "alternate") == 0) {
        for (int i = 0; i < N; i += 2)
            w[i] = 0;
    } else {
        fprintf(stderr, "usage: %s none|front|random|alternate\n", argv[0]);
        return 2;
    }

    double best = 1e30;
    for (int pass = 0; pass < PASSES; pass++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        sink = dot_skip(w, x, N);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
    }
    printf("%-9s %4.2f ns per element\n", input, best / N);
    return 0;
}
