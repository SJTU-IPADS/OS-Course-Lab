/* matvec.c -- the product of a matrix with a vector, one weight at a time.

   This is matvec() of mini_ollama.c of lecture 3 (lectures/3-asm/examples/
   mini-ollama) in a file of its own, without the counter of weights.
   guest/mini-ollama is built with this file; matvec_simd.c computes the
   same product 32 weights at a time. */
#include "libc.h"
#include "mini_ollama.h"

/* y = W x, one weight at a time. A Q4_0 block holds 32 weights in 18 bytes:
   a half-precision scale d, then 16 bytes. Byte j has weight j in its low
   4 bits and weight j + 16 in its high 4 bits; a stored value q means the
   weight d * (q - 8). */
void matvec(float *y, const struct tensor *w, const float *x)
{
    const uint8_t *blk = w->data;
    for (size_t r = 0; r < w->rows; r++) {
        float sum = 0;
        for (size_t c = 0; c < w->cols; c += 32, blk += 18) {
            float d = fp16[blk[0] | blk[1] << 8];
            for (int j = 0; j < 16; j++) {
                int lo = (blk[2 + j] & 15) - 8, hi = (blk[2 + j] >> 4) - 8;
                sum += d * lo * x[c + j] + d * hi * x[c + j + 16];
            }
        }
        y[r] = sum;
    }
}
