/* The linear function, sum of w[i] * x[i] plus b, computed by the scalar
 * loop and by the loop on ymm registers (guest/linear.s; its vector loop
 * is the AVX2 loop of the SIMD section of lecture 3).
 *
 *   simd          n = 19: two vector iterations and a tail of three
 *   simd x        n = 1000
 *
 * The two lines must agree; the vector loop uses the AVX2 subset of lab 1.
 */
#include "mini.h"

int linear_scalar(const int *w, const int *x, int b, int n);
int linear_simd(const int *w, const int *x, int b, int n);

static int w[1000], x[1000];

int guest_main(int argc, char **argv)
{
    (void)argv;
    int n = argc > 1 ? 1000 : 19;
    for (int i = 0; i < n; ++i) {
        w[i] = i % 7 - 3;
        x[i] = 2 * i + 1;
    }
    puts("scalar = ");
    putint(linear_scalar(w, x, 5, n));
    puts("simd   = ");
    putint(linear_simd(w, x, 5, n));
    return 0;
}
