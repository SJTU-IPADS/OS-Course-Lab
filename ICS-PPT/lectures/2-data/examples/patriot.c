/* The Patriot clock: 0.1 s stored as a 23-bit binary fraction, error accumulated over 100 hours. */
#include <stdio.h>

int main(void) {
    double x = (double) (unsigned) (0.1 * (1 << 23)) / (1 << 23);   /* chop 0.1 after 23 bits */
    double err = 0.1 - x;
    long ticks = 100L * 3600 * 10;                                   /* one tick every 0.1 s */
    double drift = err * ticks;

    printf("x (23 bits)       %.10f\n", x);
    printf("0.1 - x           %.3g s\n", err);
    printf("ticks in 100 h    %ld\n", ticks);
    printf("clock error       %.3f s\n", drift);
    printf("Scud at 1676 m/s  %.1f m\n", drift * 1676);
    return 0;
}
