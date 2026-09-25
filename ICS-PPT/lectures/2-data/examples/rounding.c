/* A float takes the nearest representable value; ties go to the even one. */
#include <stdio.h>

int main(void) {
    /* Above 2^24 the spacing between floats is 2, so odd values are ties. */
    printf("16777216 as float   %.1f\n", (double) 16777216.0f);
    printf("16777217 as float   %.1f\n", (double) 16777217.0f);
    printf("16777219 as float   %.1f\n", (double) 16777219.0f);

    /* 0.1 is 0.0001100110011... in binary, so it never fits exactly. */
    printf("0.1 as double       %.20f\n", 0.1);
    printf("0.1 as float        %.20f\n", (double) 0.1f);
    return 0;
}
