/* Decode a few BF16 bit patterns with the compiler's own __bf16.
   __bf16 needs gcc 12 or newer on x86-64. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void show(uint16_t h) {
    __bf16 v;
    memcpy(&v, &h, 2);
    printf("0x%04x   %d ", h, h >> 15);
    for (int i = 14; i >= 7; i--) putchar((h >> i & 1) ? '1' : '0');
    putchar(' ');
    for (int i = 6; i >= 0; i--) putchar((h >> i & 1) ? '1' : '0');
    printf("   %.9g\n", (double) v);
}

int main(void) {
    show(0x0000);   /* +0 */
    show(0x0001);   /* smallest denormalized */
    show(0x007f);   /* largest denormalized */
    show(0x0080);   /* smallest normalized */
    show(0x3f80);   /* 1.0 */
    show(0x7f7f);   /* largest normalized */
    show(0x7f80);   /* +inf */
    show(0x7fc0);   /* NaN */
    return 0;
}
