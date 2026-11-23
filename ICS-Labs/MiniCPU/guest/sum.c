/* One dependency chain: the sum of 1..limit. */
#include "mini.h"

int guest_main(int argc, char **argv)
{
    (void)argv;
    long limit = argc > 1 ? 1000 : 100;
    long total = 0;
    for (long number = 1; number <= limit; ++number)
        total += number;
    puts("sum = ");
    putint(total);
    return 0;
}
