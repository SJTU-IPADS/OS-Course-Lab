/* Floating point does not obey the laws of real arithmetic. */
#include <stdio.h>

int main(void) {
    printf("(3.14+1e20)-1e20          %g\n", (3.14 + 1e20) - 1e20);
    printf("3.14+(1e20-1e20)          %g\n", 3.14 + (1e20 - 1e20));
    printf("0.1 + 0.2 == 0.3          %d\n", 0.1 + 0.2 == 0.3);
    return 0;
}
