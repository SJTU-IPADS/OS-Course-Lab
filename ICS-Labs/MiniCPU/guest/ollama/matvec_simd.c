/* matvec_simd.c -- the product of a matrix with a vector, 32 weights at a
   time: the way ollama computes it.

   guest/mini-ollama-simd is built with this file in place of matvec.c. x is
   first rounded to integers, 32 at a time, each block of 32 with a scale of
   its own (the Q8_0 format of ollama). The 32 weights of a Q4_0 block are
   d * (q - 8) and the 32 numbers of x they meet are close to s * xi, so the
   32 products add up to

       d * s * (sum of q * xi  -  8 * sum of xi)

   The first sum is over integers: dot_q4 (dot_q4.s) computes it eight
   products at a time on ymm registers. The second sum is the same for
   every row and is computed once. Because x is rounded, the result differs
   from that of matvec.c from about the third digit on. */
#include "libc.h"
#include "mini_ollama.h"

/* The sum over j = 0 .. 15 of (b[j] & 15) * x[j] + (b[j] >> 4) * x[j + 16]:
   b is the 16 bytes of a Q4_0 block, x has 32 integers. */
int dot_q4(const unsigned char *b, const int *x);

/* v rounded to the nearest integer, a half away from zero. */
static int nearest(float v)
{
    int n = (int)v;                     /* the fraction is cut off */
    float rest = v - (float)n;
    if (rest >= 0.5f)
        n++;
    if (rest <= -0.5f)
        n--;
    return n;
}

/* x[0..32) as integers xi[0..32) from -127 to 127: x[j] is close to
   scale * xi[j], where scale is the largest |x[j]| / 127. Returns scale and
   leaves the sum of the integers in *sum. A block of zeros has scale 0. */
static float to_integers(int *xi, const float *x, int *sum)
{
    float max = 0, min = 0;
    for (int j = 0; j < 32; j++) {
        if (x[j] > max)
            max = x[j];
        if (x[j] < min)
            min = x[j];
    }
    if (-min > max)
        max = -min;                     /* the largest |x[j]| */

    float scale = max / 127;
    *sum = 0;
    for (int j = 0; j < 32; j++) {
        xi[j] = scale > 0 ? nearest(x[j] / scale) : 0;
        *sum += xi[j];
    }
    return scale;
}

/* y = W x, a block of 32 weights at a time. A Q4_0 block holds 32 weights in
   18 bytes: a half-precision scale d, then 16 bytes. Byte j has weight j in
   its low 4 bits and weight j + 16 in its high 4 bits; a stored value q
   means the weight d * (q - 8). */
void matvec(float *y, const struct tensor *w, const float *x)
{
    size_t n = w->cols / 32;            /* blocks in a row */
    int xi[w->cols], xsum[n];
    float xscale[n];

    for (size_t i = 0; i < n; i++)
        xscale[i] = to_integers(xi + 32 * i, x + 32 * i, &xsum[i]);

    const uint8_t *blk = w->data;
    for (size_t r = 0; r < w->rows; r++) {
        float sum = 0;
        for (size_t i = 0; i < n; i++, blk += 18) {
            float d = fp16[blk[0] | blk[1] << 8];
            sum += d * xscale[i] * (float)(dot_q4(blk + 2, xi + 32 * i) - 8 * xsum[i]);
        }
        y[r] = sum;
    }
}
