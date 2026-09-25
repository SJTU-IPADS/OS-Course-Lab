/* One Q4_0 block: 32 floats -> one fp16 scale plus 32 packed 4-bit codes. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define N 32

typedef struct {          /* 18 bytes: this is llama.cpp's block_q4_0 */
    uint16_t d;           /* the scale, stored as fp16 */
    uint8_t  qs[N / 2];   /* two codes per byte */
} block_q4_0;

/* fp16 is used here only to store one number, so a minimal encoder is
   enough: no subnormals, no infinities. */
static uint16_t f32_to_f16(float f) {
    uint32_t b;
    __builtin_memcpy(&b, &f, 4);
    uint32_t sign = (b >> 16) & 0x8000;
    int32_t  exp  = (int32_t) ((b >> 23) & 0xff) - 127 + 15;
    uint32_t man  = (b >> 13) & 0x3ff;
    if (exp <= 0) return (uint16_t) sign;
    return (uint16_t) (sign | (uint32_t) exp << 10 | man);
}

static float f16_to_f32(uint16_t h) {
    uint32_t sign = (uint32_t) (h & 0x8000) << 16;
    uint32_t exp  = (h >> 10) & 0x1f;
    uint32_t man  = h & 0x3ff;
    if (exp == 0) return 0.0f;
    uint32_t b = sign | (exp - 15 + 127) << 23 | man << 13;
    float f;
    __builtin_memcpy(&f, &b, 4);
    return f;
}

/* The block's extreme value, sign kept. Dividing by -8 maps it to code 0,
   so all sixteen codes are reachable; dividing by absmax/7 would waste one. */
static float extreme(const float *x, int n) {
    float amax = 0.0f, m = 0.0f;
    for (int i = 0; i < n; i++)
        if (fabsf(x[i]) > amax) { amax = fabsf(x[i]); m = x[i]; }
    return m;
}

static int clamp(long q) { return q < 0 ? 0 : q > 15 ? 15 : (int) q; }

static void quantize(const float *x, block_q4_0 *out) {
    float d  = extreme(x, N) / -8.0f;          /* codes 0..15 mean -8..7 */
    float id = d ? 1.0f / d : 0.0f;
    out->d = f32_to_f16(d);                    /* the scale is stored as fp16 */

    for (int j = 0; j < N / 2; j++) {          /* weight j and weight j + 16 */
        int lo = clamp((long) (x[j]         * id + 8.5f));
        int hi = clamp((long) (x[j + N / 2] * id + 8.5f));
        out->qs[j] = (uint8_t) (hi << 4 | lo); /* two weights, one byte */
    }
}

static void dequantize(const block_q4_0 *in, float *x) {
    float d = f16_to_f32(in->d);
    for (int j = 0; j < N / 2; j++) {
        x[j]         = ((in->qs[j] & 0x0f) - 8) * d;
        x[j + N / 2] = ((in->qs[j] >> 4)   - 8) * d;
    }
}

int main(void) {
    float w[N], back[N];
    for (int i = 0; i < N; i++)
        w[i] = 0.35f * sinf(0.7f * i + 0.9f) + 0.02f * i;  /* stand-in weights */

    block_q4_0 blk;
    quantize(w, &blk);
    dequantize(&blk, back);

    float amax = 0.0f, emax = 0.0f;
    for (int i = 0; i < N; i++) {
        if (fabsf(w[i]) > amax) amax = fabsf(w[i]);
        if (fabsf(w[i] - back[i]) > emax) emax = fabsf(w[i] - back[i]);
    }

    printf("first four weights   %+.4f %+.4f %+.4f %+.4f\n", w[0], w[1], w[2], w[3]);
    printf("after quantization   %+.4f %+.4f %+.4f %+.4f\n",
           back[0], back[1], back[2], back[3]);
    printf("qs[0] = 0x%02x         low -> w[0] code %u, high -> w[16] code %u\n",
           blk.qs[0], blk.qs[0] & 0x0f, blk.qs[0] >> 4);
    printf("bytes                %zu -> %zu   (%.2f bits per weight)\n",
           sizeof(w), sizeof(blk), 8.0 * sizeof(blk) / N);
    printf("largest error        %.4f  (%.1f%% of the largest weight)\n",
           emax, 100.0 * emax / amax);
    return 0;
}
