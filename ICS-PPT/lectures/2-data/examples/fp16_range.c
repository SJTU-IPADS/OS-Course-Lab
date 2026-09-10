/* Range comes from the exponent field, precision from the mantissa.
   _Float16 and __bf16 need gcc 12 or newer on x86-64. */
#include <stdio.h>

int main(void) {
    printf("65504     as fp16   %g\n", (double) (_Float16) 65504.0f);
    printf("100000    as fp16   %g\n", (double) (_Float16) 100000.0f);
    printf("100000    as bf16   %g\n", (double) (__bf16)   100000.0f);
    printf("1.0/3     as fp32   %.8f\n", 1.0f / 3.0f);
    printf("1.0/3     as fp16   %.8f\n", (double) (_Float16) (1.0f / 3.0f));
    printf("1.0/3     as bf16   %.8f\n", (double) (__bf16)   (1.0f / 3.0f));
    return 0;
}
