/* A number in binary, then the same number with the point moved to one place. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t bits(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }

/* |x| written in binary, at most `frac` places after the point. */
static void binary_of(char *out, float x, int frac) {
    char *p = out;
    if (x < 0) { *p++ = '-'; x = -x; }
    uint32_t whole = (uint32_t) x;
    float rest = x - (float) whole;
    char tmp[40]; int n = 0;
    do { tmp[n++] = (char) ('0' + (whole & 1)); whole >>= 1; } while (whole);
    while (n) *p++ = tmp[--n];
    if (rest > 0.0f) {
        *p++ = '.';
        for (int i = 0; i < frac && rest > 0.0f; i++) {
            rest *= 2.0f;
            *p++ = (rest >= 1.0f) ? '1' : '0';
            if (rest >= 1.0f) rest -= 1.0f;
        }
        if (rest > 0.0f) { *p++ = '.'; *p++ = '.'; }
    }
    *p = '\0';
}

static void show(float x) {
    uint32_t b = bits(x);
    unsigned e = b >> 23 & 0xff, f = b & 0x7fffffu;
    char plain[64], mant[32], moved[64];
    binary_of(plain, x, 12);

    int last = 0;                            /* trailing zeros are not written */
    for (int i = 0; i < 23; i++) { mant[i] = (char) ('0' + (f >> (22 - i) & 1));
                                   if (mant[i] == '1') last = i + 1; }
    if (last > 10) { mant[10] = mant[11] = '.'; last = 12; }
    mant[last] = '\0';
    snprintf(moved, sizeof moved, "%s1.%s x 2^%d", x < 0 ? "-" : "", mant,
             (int) e - 127);

    printf("%-9g %-17s %-22s e %3u = 127%+d\n", x, plain, moved, e, (int) e - 127);
}

int main(void) {
    printf("%-9s %-16s %-21s %s\n", "value", "in binary", "point moved", "stored");
    show(6.5f);
    show(0.75f);
    show(-5.0f);
    show(0.1f);
    return 0;
}
