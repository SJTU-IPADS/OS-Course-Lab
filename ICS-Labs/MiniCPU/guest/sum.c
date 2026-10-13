/* One dependency chain: the sum of 1..n. Lecture 4 measures the CPI of this loop. */
#include "mini.h"

int guest_main(int argc, char **argv)
{
    (void)argv;
    long n = argc > 1 ? 1000 : 100;
    long s = 0;
    for (long i = 1; i <= n; ++i)
        s += i;
    puts("sum = ");
    putint(s);
    return 0;
}
