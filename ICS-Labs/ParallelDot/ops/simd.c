/* simd.c -- the operators of mini-ollama with AVX2 intrinsics, 8 floats per
 * step, on the CPU. All the code you write in step 1 goes in this file.
 *
 * Everything below is written except matvec_q4_0(), the product of a
 * Q4_0 matrix and a vector: replace its body (the (void) line and the
 * todo() call) with your implementation. What each operator computes is in
 * ops.h. The other operators show the intrinsics at work and can be read
 * as examples; hsum8() is the horizontal sum that matvec_q4_0 also needs
 * at the end of each row.
 *
 *   make                  build mini-ollama-scalar, mini-ollama-simd and
 *                         ops-selftest-simd
 *   ./ops-selftest-simd   check every operator against ops/scalar.c, then
 *                         time matvec against the scalar one
 *   make run-simd         answer prompt.txt with the model, with these
 *                         operators
 *   make run-scalar       the same with the scalar operators, for comparison
 *
 * This file is compiled with -mavx2: the AVX2 intrinsics declared in
 * <immintrin.h> can be used here.
 *
 * A vector whose length is not a multiple of 8 ends with fewer than 8
 * floats. The operators read and write such a tail with masked loads and
 * stores (_mm256_maskload_ps, _mm256_maskstore_ps): a lane outside the mask
 * reads as 0 and is not written.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>

#include "ops.h"

/* A function that is not written yet calls todo("name"): the program then
 * reports the name and exits with status 3. It is static inline, so that
 * -Wall does not report it once no function calls it. */
static inline void todo(const char *name)
{
    fprintf(stderr, "%s is not implemented yet\n", name);
    exit(3);
}

/* The sum of the 8 floats of v: the high 128 bits are added to the low 128
 * bits (8 floats to 4), then two horizontal adds (4 to 2 to 1). */
static inline float hsum8(__m256 v)
{
    __m128 s = _mm_add_ps(_mm256_castps256_ps128(v),       /* the low 4 floats */
                          _mm256_extractf128_ps(v, 1));    /* plus the high 4 */
    s = _mm_hadd_ps(s, s);                                 /* 4 to 2 */
    s = _mm_hadd_ps(s, s);                                 /* 2 to 1, in the low 32 bits */
    return _mm_cvtss_f32(s);
}

/* The largest of the 8 floats of v, the same way. */
static inline float hmax8(__m256 v)
{
    __m128 m = _mm_max_ps(_mm256_castps256_ps128(v), _mm256_extractf128_ps(v, 1));
    m = _mm_max_ps(m, _mm_movehl_ps(m, m));                /* lanes 2, 3 onto 0, 1 */
    m = _mm_max_ss(m, _mm_shuffle_ps(m, m, 1));            /* lane 1 onto 0 */
    return _mm_cvtss_f32(m);
}

/* The mask of the first k lanes (k < 8): lane i is all ones when i < k. */
static inline __m256i tail_mask(int k)
{
    return _mm256_cmpgt_epi32(_mm256_set1_epi32(k), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
}

/* e^x of 8 floats. x is split into n * ln 2 + r with n an integer and
 * |r| <= ln 2 / 2; e^r is a polynomial of degree 7 in r (the coefficients
 * of the Cephes library), and 2^n is made by writing n + 127 into the
 * exponent bits of a float. For x in [-87, 88] the result is within 1 unit
 * in the last place of e^x rounded to float. For x below -87 it is 0, for
 * x above 88 infinity; e^x is below 1.7e-38 or above 1.6e38 there. */
static inline __m256 exp8(__m256 x)
{
    __m256 under = _mm256_cmp_ps(x, _mm256_set1_ps(-87.0f), _CMP_LT_OQ);
    __m256 over = _mm256_cmp_ps(x, _mm256_set1_ps(88.0f), _CMP_GT_OQ);
    x = _mm256_min_ps(x, _mm256_set1_ps(88.0f));
    x = _mm256_max_ps(x, _mm256_set1_ps(-87.0f));
    __m256 n = _mm256_round_ps(_mm256_mul_ps(x, _mm256_set1_ps(1.44269504088896341f)),
                               _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
    /* r = x - n ln 2, with ln 2 in two parts so that n * 0.693359375 is exact */
    __m256 r = _mm256_sub_ps(x, _mm256_mul_ps(n, _mm256_set1_ps(0.693359375f)));
    r = _mm256_sub_ps(r, _mm256_mul_ps(n, _mm256_set1_ps(-2.12194440e-4f)));

    __m256 p = _mm256_set1_ps(1.9875691500e-4f);
    p = _mm256_add_ps(_mm256_mul_ps(p, r), _mm256_set1_ps(1.3981999507e-3f));
    p = _mm256_add_ps(_mm256_mul_ps(p, r), _mm256_set1_ps(8.3334519073e-3f));
    p = _mm256_add_ps(_mm256_mul_ps(p, r), _mm256_set1_ps(4.1665795894e-2f));
    p = _mm256_add_ps(_mm256_mul_ps(p, r), _mm256_set1_ps(1.6666665459e-1f));
    p = _mm256_add_ps(_mm256_mul_ps(p, r), _mm256_set1_ps(5.0000001201e-1f));
    p = _mm256_add_ps(_mm256_mul_ps(_mm256_mul_ps(p, r), r), _mm256_add_ps(r, _mm256_set1_ps(1.0f)));

    __m256i e = _mm256_slli_epi32(_mm256_add_epi32(_mm256_cvtps_epi32(n), _mm256_set1_epi32(127)), 23);
    p = _mm256_mul_ps(p, _mm256_castsi256_ps(e));
    p = _mm256_andnot_ps(under, p);                                  /* 0 below -87 */
    return _mm256_blendv_ps(p, _mm256_set1_ps(INFINITY), over);      /* infinity above 88 */
}

/* y = W x for a matrix W of rows x cols weights in Q4_0, 32 weights per
 * step.
 *
 * Row r of W is the cols / 32 blocks at w + r * (cols / 32 * 18); x points
 * to cols floats; cols is a positive multiple of 32. y[r] is the sum over
 * c < cols of W[r][c] * x[c], computed in float. The order of the additions
 * is free. Read only the rows * cols / 32 blocks of W and the cols floats
 * of x. Neither w nor x is aligned to 16 or 32 bytes: use the unaligned
 * loads.
 *
 * TODO: replace the two lines of the body with the AVX2 version. One way:
 * for each row, keep 8 partial sums of the row in one register, and for
 * each block of 32 weights of the row:
 *   1. load the 16 code bytes with one 128-bit load (_mm_loadu_si128);
 *   2. split them into the 16 low codes (weights 0..15) and the 16 high
 *      codes (weights 16..31): an AND with 0x0f, and a shift right by 4
 *      followed by the same AND (_mm_and_si128, _mm_srli_epi16);
 *   3. widen 8 codes at a time to 8 32-bit integers (_mm256_cvtepu8_epi32),
 *      subtract 8 (_mm256_sub_epi32), convert to 8 floats
 *      (_mm256_cvtepi32_ps);
 *   4. multiply by the 8 floats of x they go with (_mm256_loadu_ps,
 *      _mm256_mul_ps) and add to the sum of the block (_mm256_add_ps);
 *   5. multiply the sum of the block by d (_mm256_set1_ps) and add it to
 *      the 8 partial sums of the row.
 * After the last block of the row, y[r] is the sum of the 8 partial sums
 * (hsum8). */
static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols)
{
    (void)y; (void)w; (void)x; (void)rows; (void)cols;
    todo("matvec_q4_0");
}

/* Eight codes (the low 8 bytes of q) as the floats d * (q - 8). */
static inline __m256 decode8(__m128i q, __m256 d)
{
    __m256i wide = _mm256_sub_epi32(_mm256_cvtepu8_epi32(q), _mm256_set1_epi32(8));
    return _mm256_mul_ps(d, _mm256_cvtepi32_ps(wide));
}

/* A block per step: the 16 code bytes become 32 weights, 8 per store. */
static void embed(float *x, const uint8_t *blk, size_t n)
{
    const __m128i low4 = _mm_set1_epi8(15);
    for (size_t c = 0; c < n; c += 32, blk += 18) {
        __m256 d = _mm256_set1_ps(q4_0_scale(blk));
        __m128i bytes = _mm_loadu_si128((const __m128i *)(blk + 2));
        __m128i lo = _mm_and_si128(bytes, low4);                      /* weights 0..15 */
        __m128i hi = _mm_and_si128(_mm_srli_epi16(bytes, 4), low4);   /* weights 16..31 */
        _mm256_storeu_ps(x + c,      decode8(lo, d));
        _mm256_storeu_ps(x + c + 8,  decode8(_mm_srli_si128(lo, 8), d));
        _mm256_storeu_ps(x + c + 16, decode8(hi, d));
        _mm256_storeu_ps(x + c + 24, decode8(_mm_srli_si128(hi, 8), d));
    }
}

/* The sum of a[i] * b[i] over i < n: 8 partial sums, then hsum8. */
static float dot(const float *a, const float *b, int n)
{
    __m256 acc = _mm256_setzero_ps();
    int i = 0;
    for (; i + 8 <= n; i += 8)
        acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_loadu_ps(a + i), _mm256_loadu_ps(b + i)));
    if (i < n) {
        __m256i m = tail_mask(n - i);
        acc = _mm256_add_ps(acc, _mm256_mul_ps(_mm256_maskload_ps(a + i, m),
                                               _mm256_maskload_ps(b + i, m)));
    }
    return hsum8(acc);
}

/* The sum of squares is dot(x, x); the scaling is 8 floats per step. */
static void rmsnorm(float *y, const float *x, const float *gain, int n_vec, int d, float eps)
{
    for (int v = 0; v < n_vec; v++, x += d, y += d) {
        __m256 scale = _mm256_set1_ps(1 / sqrtf(dot(x, x, d) / d + eps));
        int i = 0;
        for (; i + 8 <= d; i += 8)
            _mm256_storeu_ps(y + i, _mm256_mul_ps(_mm256_mul_ps(_mm256_loadu_ps(x + i), scale),
                                                  _mm256_loadu_ps(gain + i)));
        if (i < d) {
            __m256i m = tail_mask(d - i);
            _mm256_maskstore_ps(y + i, m, _mm256_mul_ps(_mm256_mul_ps(_mm256_maskload_ps(x + i, m), scale),
                                                        _mm256_maskload_ps(gain + i, m)));
        }
    }
}

/* The d/2 angles are the same for every vector: their cosines and sines
 * are computed once, with the C library, one at a time. The turning is 8
 * pairs per step. */
static void rope(float *v, int n_vec, int d, int pos, float base)
{
    int half = d / 2;
    float cs[half], sn[half];
    for (int i = 0; i < half; i++) {
        float angle = pos * powf(base, -2.0f * i / d);
        cs[i] = cosf(angle);
        sn[i] = sinf(angle);
    }
    for (int h = 0; h < n_vec; h++, v += d) {
        int i = 0;
        for (; i + 8 <= half; i += 8) {
            __m256 c = _mm256_loadu_ps(cs + i), s = _mm256_loadu_ps(sn + i);
            __m256 a = _mm256_loadu_ps(v + i), b = _mm256_loadu_ps(v + half + i);
            _mm256_storeu_ps(v + i,        _mm256_sub_ps(_mm256_mul_ps(a, c), _mm256_mul_ps(b, s)));
            _mm256_storeu_ps(v + half + i, _mm256_add_ps(_mm256_mul_ps(b, c), _mm256_mul_ps(a, s)));
        }
        for (; i < half; i++) {                         /* fewer than 8 pairs left */
            float a = v[i], b = v[i + half];
            v[i]        = a * cs[i] - b * sn[i];
            v[i + half] = b * cs[i] + a * sn[i];
        }
    }
}

static void attn_scores(float *s, const float *q, const float *k_cache,
                        int n_head, int n_head_kv, int d, int n_tok)
{
    int kv = n_head_kv * d;
    for (int h = 0; h < n_head; h++) {
        int off = h / (n_head / n_head_kv) * d;         /* its key head */
        for (int t = 0; t < n_tok; t++)
            s[h * n_tok + t] = dot(q + h * d, k_cache + (size_t)t * kv + off, d) / sqrtf(d);
    }
}

/* Per row: the largest score, then 8 exponentials per step (exp8) and
 * their sum, then the division. */
static void softmax(float *s, int n_row, int n)
{
    for (int r = 0; r < n_row; r++, s += n) {
        int i, k = n % 8;                               /* k floats in the tail */
        __m256i m = tail_mask(k);
        __m256 vmax = _mm256_set1_ps(s[0]);
        for (i = 0; i + 8 <= n; i += 8)
            vmax = _mm256_max_ps(vmax, _mm256_loadu_ps(s + i));
        if (k)
            vmax = _mm256_max_ps(vmax, _mm256_blendv_ps(vmax, _mm256_maskload_ps(s + i, m),
                                                         _mm256_castsi256_ps(m)));
        __m256 max = _mm256_set1_ps(hmax8(vmax)), sum = _mm256_setzero_ps();
        for (i = 0; i + 8 <= n; i += 8) {
            __m256 e = exp8(_mm256_sub_ps(_mm256_loadu_ps(s + i), max));
            _mm256_storeu_ps(s + i, e);
            sum = _mm256_add_ps(sum, e);
        }
        if (k) {
            __m256 e = exp8(_mm256_sub_ps(_mm256_maskload_ps(s + i, m), max));
            e = _mm256_and_ps(e, _mm256_castsi256_ps(m));   /* lanes past the end add 0 */
            _mm256_maskstore_ps(s + i, m, e);
            sum = _mm256_add_ps(sum, e);
        }
        __m256 total = _mm256_set1_ps(hsum8(sum));
        for (i = 0; i + 8 <= n; i += 8)
            _mm256_storeu_ps(s + i, _mm256_div_ps(_mm256_loadu_ps(s + i), total));
        if (k)
            _mm256_maskstore_ps(s + i, m, _mm256_div_ps(_mm256_maskload_ps(s + i, m), total));
    }
}

/* For each token, its score times its value is added to the output of the
 * head, 8 floats per step. */
static void attn_values(float *out, const float *s, const float *v_cache,
                        int n_head, int n_head_kv, int d, int n_tok)
{
    int kv = n_head_kv * d, k = d % 8;
    __m256i m = tail_mask(k);
    for (int h = 0; h < n_head; h++) {
        int off = h / (n_head / n_head_kv) * d;
        float *o = out + h * d;
        for (int j = 0; j < d; j++)
            o[j] = 0;
        for (int t = 0; t < n_tok; t++) {
            __m256 w = _mm256_set1_ps(s[h * n_tok + t]);
            const float *v = v_cache + (size_t)t * kv + off;
            int j = 0;
            for (; j + 8 <= d; j += 8)
                _mm256_storeu_ps(o + j, _mm256_add_ps(_mm256_loadu_ps(o + j),
                                                      _mm256_mul_ps(w, _mm256_loadu_ps(v + j))));
            if (k)
                _mm256_maskstore_ps(o + j, m, _mm256_add_ps(_mm256_maskload_ps(o + j, m),
                                                            _mm256_mul_ps(w, _mm256_maskload_ps(v + j, m))));
        }
    }
}

static inline __m256 swiglu8(__m256 g, __m256 u)
{
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 e = exp8(_mm256_sub_ps(_mm256_setzero_ps(), g));          /* e^-g */
    return _mm256_mul_ps(_mm256_div_ps(g, _mm256_add_ps(one, e)), u);
}

static void swiglu(float *gate, const float *up, int n)
{
    int i = 0;
    for (; i + 8 <= n; i += 8)
        _mm256_storeu_ps(gate + i, swiglu8(_mm256_loadu_ps(gate + i), _mm256_loadu_ps(up + i)));
    if (i < n) {
        __m256i m = tail_mask(n - i);
        _mm256_maskstore_ps(gate + i, m, swiglu8(_mm256_maskload_ps(gate + i, m),
                                                 _mm256_maskload_ps(up + i, m)));
    }
}

static void add(float *x, const float *y, int n)
{
    int i = 0;
    for (; i + 8 <= n; i += 8)
        _mm256_storeu_ps(x + i, _mm256_add_ps(_mm256_loadu_ps(x + i), _mm256_loadu_ps(y + i)));
    if (i < n) {
        __m256i m = tail_mask(n - i);
        _mm256_maskstore_ps(x + i, m, _mm256_add_ps(_mm256_maskload_ps(x + i, m),
                                                    _mm256_maskload_ps(y + i, m)));
    }
}

/* Two passes: the largest value, 8 floats per step; then the first index
 * that holds it, 8 comparisons per step (_mm256_movemask_ps gives one bit
 * per lane). */
static int argmax(const float *x, int n)
{
    __m256 vmax = _mm256_set1_ps(x[0]);
    int i = 0;
    for (; i + 8 <= n; i += 8)
        vmax = _mm256_max_ps(vmax, _mm256_loadu_ps(x + i));
    float max = hmax8(vmax);
    for (; i < n; i++)
        if (x[i] > max)
            max = x[i];

    __m256 want = _mm256_set1_ps(max);
    for (i = 0; i + 8 <= n; i += 8) {
        int bits = _mm256_movemask_ps(_mm256_cmp_ps(_mm256_loadu_ps(x + i), want, _CMP_EQ_OQ));
        if (bits)
            return i + __builtin_ctz((unsigned)bits);
    }
    for (; i < n; i++)
        if (x[i] == max)
            return i;
    return 0;
}

/* ---------------- the CPU and its memory ----------------
 *
 * As in scalar.c: the memory of these operators is the memory of the
 * program. upload() returns the bytes it is given, download() copies. */

static const char *init(void)
{
    return "CPU, 1 thread, AVX2";
}

static void *alloc(size_t bytes)
{
    void *p = calloc(1, bytes ? bytes : 1);
    if (!p) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    return p;
}

static void *upload(const void *host, size_t bytes)
{
    (void)bytes;
    return (void *)host;
}

static void download(void *host, const void *mem, size_t bytes)
{
    if (host != mem)
        memcpy(host, mem, bytes);
}

static void finish(void)
{
}

const struct ops ops_simd = {
    "simd (ops/simd.c)",
    init, alloc, upload, download, finish,
    matvec_q4_0, embed, rmsnorm, rope,
    attn_scores, softmax, attn_values, swiglu, add, argmax,
};
