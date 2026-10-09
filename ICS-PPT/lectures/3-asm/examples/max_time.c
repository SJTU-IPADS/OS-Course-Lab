/* Time of max on two inputs. max.c is compiled twice and this file is
   linked with each result (see Makefile):

     make max_jl max_cmov
     ./max_jl        max compiled with -Og: selects with jl
     ./max_cmov      max compiled with -O2: selects with cmovge

   This file is compiled with -O2 both times. max is in a file of its own,
   so the calls below are not inlined.

   Input 1: a < b holds in every call.
   Input 2: a < b holds in a random half of the calls. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int max(int a, int b);

#define N (1 << 20)                     /* calls per measurement */
static int a[N], b[N];

/* Call max on all N pairs, 50 times over; return the time of the fastest
   pass in ns per call. The first 10 passes are not timed. */
static double ns_per_call(void) {
    static long total;
    double best = 1e30;
    for (int pass = -10; pass < 50; pass++) {
        struct timespec t0, t1;
        long sum = 0;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (int i = 0; i < N; i++)
            sum += max(a[i], b[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
        total += sum;
        if (pass >= 0 && ns < best)
            best = ns;
    }
    return best / N;
}

int main(int argc, char **argv) {
    (void)argc;
    for (int i = 0; i < N; i++) {       /* input 1: a = 0, b = 1 */
        a[i] = 0;
        b[i] = 1;
    }
    printf("%-10s  a < b in every call         %4.1f ns per call\n",
           argv[0], ns_per_call());

    srand(1);
    for (int i = 0; i < N; i++)         /* input 2: a = 0 or 2, b = 1 */
        a[i] = (rand() >> 8 & 1) * 2;
    printf("%-10s  a < b in a random half      %4.1f ns per call\n",
           argv[0], ns_per_call());
    return 0;
}
