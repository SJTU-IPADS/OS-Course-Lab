/* matvec.c -- the product of a matrix with a vector, one weight at a time.

   guest/mini-ollama is built with this file; matvec_simd.c computes the
   same product 32 weights at a time. */
#include "libc.h"
#include "mini_ollama.h"

/* y = W x, one weight at a time. A Q4_0 block holds 32 weights in 18 bytes:
   a half-precision scale d, then 16 bytes. Byte j has weight j in its low
   4 bits and weight j + 16 in its high 4 bits; a stored value q means the
   weight d * (q - 8). */
void matvec(float *out, const struct tensor *weights, const float *in)
{
    const uint8_t *blk = weights->data;
    for (size_t row = 0; row < weights->rows; row++) {
        float sum = 0;
        for (size_t col = 0; col < weights->cols; col += 32, blk += 18) {
            float scale = fp16[blk[0] | blk[1] << 8];
            for (int byte = 0; byte < 16; byte++) {
                int lo = (blk[2 + byte] & 15) - 8, hi = (blk[2 + byte] >> 4) - 8;
                sum += scale * lo * in[col + byte] +
                       scale * hi * in[col + byte + 16];
            }
        }
        out[row] = sum;
    }
}
