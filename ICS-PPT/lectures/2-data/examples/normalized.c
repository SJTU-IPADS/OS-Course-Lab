/* Encode 12345 as a single-precision float and show each step. */
#include <stdio.h>
#include <string.h>

static void bits(unsigned v, int n) {
    for (int i = n - 1; i >= 0; i--)
        putchar(v >> i & 1 ? '1' : '0');
}

int main(void) {
    float f = 12345.0f;
    unsigned u;
    memcpy(&u, &f, sizeof u);
    unsigned exp = u >> 23 & 0xff, frac = u & 0x7fffff;

    printf("value      %g (hex 0x%X)\n", f, 12345);
    printf("binary     "); bits(12345, 14); putchar('\n');
    printf("normalized 1."); bits(12345, 13); printf(" x 2^%d\n", (int)exp - 127);
    printf("frac       "); bits(frac, 23); putchar('\n');
    printf("exp        "); bits(exp, 8); printf(" (%u = 127 + %d)\n", exp, (int)exp - 127);
    printf("encoding   ");
    for (int i = 7; i >= 0; i--) { bits(u >> (4 * i) & 0xf, 4); putchar(i ? ' ' : '\n'); }
    printf("hex        %08X\n", u);
    return 0;
}
