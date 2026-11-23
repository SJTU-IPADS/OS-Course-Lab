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

/* The sum over j = 0 .. 15 of
   (packed[j] & 15) * input[j] + (packed[j] >> 4) * input[j + 16]:
   packed is the 16 bytes of a Q4_0 block, input has 32 integers. */
int dot_q4(const unsigned char *packed, const int *input);

/* value rounded to the nearest integer, a half away from zero. */
static int nearest(float value)
{
    int whole = (int)value;             /* the fraction is cut off */
    float rest = value - (float)whole;
    if (rest >= 0.5f)
        whole++;
    if (rest <= -0.5f)
        whole--;
    return whole;
}

/* in[0..32) as integers rounded[0..32) from -127 to 127: in[j] is close to
   scale * rounded[j], where scale is the largest |in[j]| / 127. Returns
   scale and leaves the sum of the integers in *sum. A block of zeros has
   scale 0. */
static float to_integers(int *rounded, const float *in, int *sum)
{
    float max = 0, min = 0;
    for (int idx = 0; idx < 32; idx++) {
        if (in[idx] > max)
            max = in[idx];
        if (in[idx] < min)
            min = in[idx];
    }
    if (-min > max)
        max = -min;                     /* the largest |in[j]| */

    float scale = max / 127;
    *sum = 0;
    for (int idx = 0; idx < 32; idx++) {
        rounded[idx] = scale > 0 ? nearest(in[idx] / scale) : 0;
        *sum += rounded[idx];
    }
    return scale;
}

/* y = W x, a block of 32 weights at a time. A Q4_0 block holds 32 weights in
   18 bytes: a half-precision scale d, then 16 bytes. Byte j has weight j in
   its low 4 bits and weight j + 16 in its high 4 bits; a stored value q
   means the weight d * (q - 8). */
void matvec(float *out, const struct tensor *weights, const float *in)
{
    size_t nblocks = weights->cols / 32;    /* blocks in a row */
    int rounded[weights->cols], rounded_sum[nblocks];
    float input_scale[nblocks];

    for (size_t block = 0; block < nblocks; block++)
        input_scale[block] = to_integers(rounded + 32 * block, in + 32 * block,
                                         &rounded_sum[block]);

    const uint8_t *blk = weights->data;
    for (size_t row = 0; row < weights->rows; row++) {
        float sum = 0;
        for (size_t block = 0; block < nblocks; block++, blk += 18) {
            float weight_scale = fp16[blk[0] | blk[1] << 8];
            sum += weight_scale * input_scale[block] *
                   (float)(dot_q4(blk + 2, rounded + 32 * block) - 8 * rounded_sum[block]);
        }
        out[row] = sum;
    }
}
