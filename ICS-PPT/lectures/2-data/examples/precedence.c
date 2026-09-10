/* The bitwise operators bind more loosely than the comparisons. */
#include <stdio.h>

int main(void) {
    int x = 6;                                   /* 0b0110 */
    printf("x & 1 == 0      %d      parses as x & (1 == 0)\n", x & 1 == 0);
    printf("(x & 1) == 0    %d      what was meant\n", (x & 1) == 0);
    printf("x << 1 + 2      %d     parses as x << (1 + 2)\n", x << 1 + 2);
    printf("(x << 1) + 2    %d     what was meant\n", (x << 1) + 2);
    printf("x & 3 | 4       %d      parses as (x & 3) | 4\n", x & 3 | 4);
    return 0;
}
