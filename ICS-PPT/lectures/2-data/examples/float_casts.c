/* Conversions between int, float and double in C. */
#include <stdio.h>

int main(void) {
    volatile int x = 16777217;          /* 2^24 + 1 */
    volatile float f = 0.1f;
    volatile double d = 0.1;
    volatile double pos = 2.7, neg = -2.7, big = 3e9;

    printf("x == (int)(float)x      %d\n", x == (int) (float) x);
    printf("x == (int)(double)x     %d\n", x == (int) (double) x);
    printf("f == (float)(double)f   %d\n", f == (float) (double) f);
    printf("d == (float)d           %d\n", d == (float) d);
    printf("2/3 == 2/3.0            %d\n", 2 / 3 == 2 / 3.0);
    printf("(int)2.7  (int)-2.7     %d  %d\n", (int) pos, (int) neg);
    printf("(int)3e9                %d\n", (int) big);   /* out of range: undefined in C */
    return 0;
}
