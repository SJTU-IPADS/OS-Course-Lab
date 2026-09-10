/* Finite representations do not obey the laws of arithmetic. */
#include <stdio.h>

int main(void) {
    int a = 200, b = 300, c = 400, d = 500;
    printf("200*300*400*500 as int    %d\n", a * b * c * d);
    printf("(3.14+1e20)-1e20          %g\n", (3.14 + 1e20) - 1e20);
    printf("3.14+(1e20-1e20)          %g\n", 3.14 + (1e20 - 1e20));
    printf("0.1 + 0.2 == 0.3          %d\n", 0.1 + 0.2 == 0.3);
    return 0;
}
