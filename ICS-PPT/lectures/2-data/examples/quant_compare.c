/* Five ways to put four bits under a weight, measured on real weights.

   The two .bf16 files under ext/ are slices of Llama-3.2-1B's own tensors
   (see ext/PROVENANCE.md). Each scheme quantizes them and reports the error
   it makes and the bytes it takes, so the table compares accuracy at a fixed
   width rather than one scheme at a time.

   Needs gcc 12+ on x86-64 for _Float16, same as fp16_range.c. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define QK   32           /* the block of Q4_0 and Q4_1 */
#define QK_K 256          /* the super-block of Q4_K, eight blocks of 32 */

typedef _Float16 f16;

static f16   to16(float f)  { return (f16) f; }
static float to32(f16 h)    { return (float) h; }
static int   clamp15(float v) { long q = lrintf(v); return q < 0 ? 0 : q > 15 ? 15 : (int) q; }

/* ---- reading the sample: raw little-endian BF16, no header ---------------- */

static float bf16_to_f32(uint16_t h) {
    uint32_t b = (uint32_t) h << 16;            /* BF16 is FP32's top 16 bits */
    float f;
    __builtin_memcpy(&f, &b, 4);
    return f;
}

static int load(const char *path, float *x, int max) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }
    int n = 0;
    uint8_t b[2];
    while (n < max && fread(b, 1, 2, f) == 2)
        x[n++] = bf16_to_f32((uint16_t) (b[0] | b[1] << 8));   /* low byte first */
    fclose(f);
    return n;
}

/* ---- symmetric: one scale per group, codes 0..15 mean -8..7 --------------- */

static float extreme(const float *x, int n) {
    float amax = 0.0f, m = 0.0f;
    for (int i = 0; i < n; i++)
        if (fabsf(x[i]) > amax) { amax = fabsf(x[i]); m = x[i]; }
    return m;
}

static void sym_group(const float *x, int n, float *y) {
    float d  = extreme(x, n) / -8.0f;
    d = to32(to16(d));                          /* the scale is stored as fp16 */
    float id = d ? 1.0f / d : 0.0f;
    for (int i = 0; i < n; i++)
        y[i] = (clamp15(x[i] * id + 8.0f) - 8) * d;
}

static void symmetric(const float *x, int n, float *y, int group) {
    for (int i = 0; i < n; i += group) sym_group(x + i, group, y + i);
}

/* ---- Q4_1: a min as well as a scale, codes 0..15 span [min, max] ---------- */

static void q4_1(const float *x, int n, float *y) {
    for (int i = 0; i < n; i += QK) {
        float lo = x[i], hi = x[i];
        for (int j = 1; j < QK; j++) {
            if (x[i + j] < lo) lo = x[i + j];
            if (x[i + j] > hi) hi = x[i + j];
        }
        float d = to32(to16((hi - lo) / 15.0f));
        float m = to32(to16(lo));               /* both stored as fp16 */
        float id = d ? 1.0f / d : 0.0f;
        for (int j = 0; j < QK; j++)
            y[i + j] = clamp15((x[i + j] - m) * id) * d + m;
    }
}

/* ---- Q4_K: eight sub-blocks share one super-block scale ------------------- */
/* Every 32 weights get their own scale and min, but those sixteen numbers are
   themselves quantized to 6 bits against two fp16 super-block scales, so the
   per-block overhead is 12 bytes for 256 weights instead of 64. Layout and
   packing follow llama.cpp's block_q4_K. */

typedef struct {                 /* 144 bytes for 256 weights = 4.5 bits each */
    f16     d, dmin;             /* super-block scales for the scales and mins */
    uint8_t scales[12];          /* eight 6-bit scales and eight 6-bit mins */
    uint8_t qs[QK_K / 2];        /* two codes per byte */
} block_q4_K;

static void put_scale_min(int j, uint8_t *q, uint8_t ls, uint8_t lm) {
    if (j < 4) { q[j] = ls; q[j + 4] = lm; }
    else {
        q[j + 4]  = (uint8_t) ((ls & 0xf) | (lm & 0xf) << 4);
        q[j - 4] |= (uint8_t) ((ls >> 4) << 6);       /* the two spare top bits */
        q[j]     |= (uint8_t) ((lm >> 4) << 6);
    }
}

static void get_scale_min(int j, const uint8_t *q, uint8_t *ls, uint8_t *lm) {
    if (j < 4) { *ls = q[j] & 63; *lm = q[j + 4] & 63; }
    else {
        *ls = (uint8_t) ((q[j + 4] & 0xf) | (q[j - 4] >> 6) << 4);
        *lm = (uint8_t) ((q[j + 4] >> 4)  | (q[j]     >> 6) << 4);
    }
}

static void q4_K_block(const float *x, block_q4_K *b) {
    float scale[8], mins[8], max_scale = 0.0f, max_min = 0.0f;
    for (int j = 0; j < 8; j++) {
        float lo = x[j * QK], hi = x[j * QK];
        for (int l = 1; l < QK; l++) {
            if (x[j * QK + l] < lo) lo = x[j * QK + l];
            if (x[j * QK + l] > hi) hi = x[j * QK + l];
        }
        if (lo > 0.0f) lo = 0.0f;               /* the offset is never positive */
        scale[j] = (hi - lo) / 15.0f;
        mins[j]  = -lo;                         /* stored as a magnitude */
        if (scale[j] > max_scale) max_scale = scale[j];
        if (mins[j]  > max_min)   max_min   = mins[j];
    }
    b->d    = to16(max_scale / 63.0f);          /* 6 bits for each sub-scale */
    b->dmin = to16(max_min / 63.0f);
    for (int i = 0; i < 12; i++) b->scales[i] = 0;
    float is = max_scale > 0 ? 63.0f / max_scale : 0.0f;
    float im = max_min   > 0 ? 63.0f / max_min   : 0.0f;
    for (int j = 0; j < 8; j++)
        put_scale_min(j, b->scales, (uint8_t) lrintf(is * scale[j]),
                      (uint8_t) lrintf(im * mins[j]));

    for (int j = 0; j < 8; j++) {               /* codes, against the stored scales */
        uint8_t ls, lm;
        get_scale_min(j, b->scales, &ls, &lm);
        float d = to32(b->d) * ls, m = to32(b->dmin) * lm;
        for (int l = 0; l < QK; l++) {
            int e = j * QK + l;                 /* weight e and weight e + 32 pair up */
            int q = d ? clamp15((x[e] + m) / d) : 0;
            if (e % 64 < 32) b->qs[e / 64 * 32 + e % 64] |= (uint8_t) q;
            else             b->qs[e / 64 * 32 + e % 64 - 32] |= (uint8_t) (q << 4);
        }
    }
}

static void q4_K_unblock(const block_q4_K *b, float *y) {
    for (int j = 0; j < 8; j++) {
        uint8_t ls, lm;
        get_scale_min(j, b->scales, &ls, &lm);
        float d = to32(b->d) * ls, m = to32(b->dmin) * lm;
        for (int l = 0; l < QK; l++) {
            int e = j * QK + l;
            int q = e % 64 < 32 ? (b->qs[e / 64 * 32 + e % 64] & 0x0f)
                                : (b->qs[e / 64 * 32 + e % 64 - 32] >> 4);
            y[e] = d * q - m;
        }
    }
}

static void q4_K(const float *x, int n, float *y) {
    for (int i = 0; i < n; i += QK_K) {
        block_q4_K b = {0};
        q4_K_block(x + i, &b);
        q4_K_unblock(&b, y + i);
    }
}

/* ---- the report ----------------------------------------------------------- */

static void report(const char *name, const float *x, const float *y, int n, double bytes) {
    double se = 0.0, sx = 0.0, emax = 0.0;
    for (int i = 0; i < n; i++) {
        double e = x[i] - y[i];
        se += e * e;
        sx += (double) x[i] * x[i];
        if (fabs(e) > emax) emax = fabs(e);
    }
    printf("  %-11s %7.0f  %5.2f   %.6f   %6.2f%%   %.6f\n",
           name, bytes, 8.0 * bytes / n, sqrt(se / n),
           100.0 * sqrt(se / sx), emax);
}

static void run(const char *path, int n) {
    static float x[4096], y[4096];
    n = load(path, x, n);
    float lo = x[0], hi = x[0], sx = 0.0f;
    for (int i = 0; i < n; i++) {
        if (x[i] < lo) lo = x[i];
        if (x[i] > hi) hi = x[i];
        sx += x[i] * x[i];
    }
    printf("%s   %d weights   range %+.4f .. %+.4f   rms %.5f\n",
           path, n, lo, hi, sqrtf(sx / n));
    printf("  scheme        bytes  bits/w   rmse       rel.rmse  max err\n");

    symmetric(x, n, y, n);   report("per-tensor", x, y, n, n / 2.0 + 2);
    symmetric(x, n, y, 256); report("per-256",    x, y, n, n / 2.0 + 2.0 * n / 256);
    symmetric(x, n, y, QK);  report("Q4_0",       x, y, n, n / 2.0 + 2.0 * n / QK);
    q4_1(x, n, y);           report("Q4_1",       x, y, n, n / 2.0 + 4.0 * n / QK);
    q4_K(x, n, y);           report("Q4_K",       x, y, n, 144.0 * n / QK_K);
    printf("\n");
}

int main(int argc, char **argv) {
    const char *which = argc > 1 ? argv[1] : "all";     /* down | norm | all */
    if (which[0] != 'n') run("ext/w-down-proj.bf16", 4096);
    if (which[0] != 'd') run("ext/w-final-norm.bf16", 2048);
    return 0;
}
