/* How far apart two neighbouring floats are, at several magnitudes. */
#include <math.h>
#include <stdio.h>

int main(void) {
    float xs[] = {1.0f, 2.0f, 4.0f, 1024.0f, 1e6f, 1e9f};
    printf("%-9s %-16s %-12s %s\n", "value", "next float up", "step", "step/value");
    for (unsigned i = 0; i < sizeof xs / sizeof *xs; i++) {
        float x = xs[i], next = nextafterf(x, INFINITY);
        printf("%-9g %-16.9g %-12g %.2e\n", x, next, next - x, (next - x) / x);
    }
    return 0;
}
