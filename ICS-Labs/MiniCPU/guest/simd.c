/* The linear function, sum of weights[i] * input[i] plus bias, computed by
 * the scalar loop and by the loop on ymm registers (guest/linear.s).
 *
 *   simd          count = 19: two vector iterations and a tail of three
 *   simd x        count = 1000
 *
 * The two lines must agree; the vector loop uses the AVX2 subset of lab 1.
 */
#include "mini.h"

int linear_scalar(const int *weights, const int *input, int bias, int count);
int linear_simd(const int *weights, const int *input, int bias, int count);

static int weights[1000], input[1000];

int guest_main(int argc, char **argv)
{
    (void)argv;
    int count = argc > 1 ? 1000 : 19;
    for (int idx = 0; idx < count; ++idx) {
        weights[idx] = idx % 7 - 3;
        input[idx] = 2 * idx + 1;
    }
    puts("scalar = ");
    putint(linear_scalar(weights, input, 5, count));
    puts("simd   = ");
    putint(linear_simd(weights, input, 5, count));
    return 0;
}
