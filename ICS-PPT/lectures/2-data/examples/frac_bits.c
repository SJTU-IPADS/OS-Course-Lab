/* Fraction to binary bits: repeatedly double x and take the integer part. */
#include <stdio.h>

static unsigned frac_bits(double x, int *used) {
    unsigned result_bits = 0, current_bit = 0x80000000;
    int i;
    for (i = 0; i < 32; i++) {
        x *= 2;
        if (x >= 1) {
            result_bits |= current_bit;
            if (x == 1)
                break;          /* nothing left: the fraction is exact */
            x -= 1;
        }
        current_bit >>= 1;
    }
    *used = i < 32 ? i + 1 : 32;
    return result_bits;
}

int main(void) {
    double xs[] = {0.75, 0.625, 0.2};
    for (unsigned k = 0; k < sizeof xs / sizeof *xs; k++) {
        int used;
        unsigned bits = frac_bits(xs[k], &used);
        printf("%-6g 0.", xs[k]);
        for (int i = 0; i < used; i++)
            putchar(bits >> (31 - i) & 1 ? '1' : '0');
        printf("%s\n", used == 32 ? " ..." : "");
    }
    return 0;
}
