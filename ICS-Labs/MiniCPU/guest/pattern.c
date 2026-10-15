/* A branch whose outcome repeats with a fixed period, for the branch
 * predictor extension.
 *
 * The pattern is taken, taken, not taken, not taken (period 4). A two-bit
 * counter indexed only by address looks up the same entry for every
 * execution of the branch and cannot predict it; a two-level predictor with
 * global history can. */
#include "mini.h"

int guest_main(int argc, char **argv)
{
    (void)argv;
    long n = argc > 1 ? 4000 : 400;
    long s = 0;
    for (long i = 0; i < n; ++i) {
        if ((i & 3) < 2)                /* taken taken not-taken not-taken */
            s += i;
        else
            s -= 1;
    }
    puts("s = ");
    putint(s);
    return 0;
}
