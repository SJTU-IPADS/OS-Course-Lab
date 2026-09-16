/* Decode three single-precision bit patterns whose exp field is all zeros. */
#include <stdio.h>
#include <string.h>

static void bits(unsigned v, int n) {
    for (int i = n - 1; i >= 0; i--)
        putchar(v >> i & 1 ? '1' : '0');
}

int main(void) {
    unsigned pats[] = {0x80000000, 0x00400000, 0x00000001};
    printf("%-10s %-2s %-8s %-23s  %-10s %-5s %s\n", "bits", "s", "exp", "frac", "M", "E", "as float");
    for (unsigned i = 0; i < sizeof pats / sizeof *pats; i++) {
        unsigned u = pats[i], frac = u & 0x7fffff;
        float f;
        memcpy(&f, &u, sizeof f);
        double m = frac / 8388608.0;        /* 0.frac: no implied leading 1 */
        printf("%08X   %-2u ", u, u >> 31);
        bits(u >> 23 & 0xff, 8); putchar(' ');
        bits(frac, 23);
        printf("  %-10.4g %-5d %g\n", m, 1 - 127, f);
    }
    return 0;
}
