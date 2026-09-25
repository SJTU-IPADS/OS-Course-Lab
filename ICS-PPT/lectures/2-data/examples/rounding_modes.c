/* Round five values to integers under each of the four IEEE rounding modes. */
#include <stdio.h>
#include <fenv.h>
#include <math.h>

int main(void) {
    static const int modes[] = { FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO, FE_TONEAREST };
    static const char *names[] = { "downward", "upward", "toward zero", "to nearest even" };
    volatile double v[] = { 1.40, 1.60, 1.50, 2.50, -1.50 };

    printf("%-16s", "mode");
    for (int j = 0; j < 5; j++) printf("%7.2f", v[j]);
    putchar('\n');
    for (int i = 0; i < 4; i++) {
        fesetround(modes[i]);
        printf("%-16s", names[i]);
        for (int j = 0; j < 5; j++) printf("%7.0f", nearbyint(v[j]));   /* uses the current mode */
        putchar('\n');
    }
    fesetround(FE_TONEAREST);
    return 0;
}
