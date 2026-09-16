/* Decode a few FP16 bit patterns with the compiler's own _Float16.
   _Float16 needs gcc 12 or newer on x86-64. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void show(uint16_t h) {
    _Float16 v;
    memcpy(&v, &h, 2);
    printf("0x%04x   %d %d%d%d%d%d ", h, h >> 15, h >> 14 & 1, h >> 13 & 1,
           h >> 12 & 1, h >> 11 & 1, h >> 10 & 1);
    for (int i = 9; i >= 0; i--) putchar((h >> i & 1) ? '1' : '0');
    printf("   %.9g\n", (double) v);
}

int main(void) {
    show(0x0001);   /* smallest subnormal */
    show(0x03ff);   /* largest subnormal */
    show(0x0400);   /* smallest normal */
    show(0x3c00);   /* 1.0 */
    show(0x7bff);   /* largest normal */
    show(0x7c00);   /* +inf */
    show(0x7e00);   /* NaN */
    return 0;
}
