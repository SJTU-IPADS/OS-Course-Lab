/* scalar.c -- the operators of the lecture's mini_ollama, one element at a
 * time, on the CPU.
 *
 * Each function computes what mini_ollama.c of lecture 3 computes, in the
 * same order of operations, so mini-ollama-scalar gives the lecture
 * program's answer. The Makefile compiles this file with
 * -fno-tree-vectorize: GCC 12 and later vectorize some loops at -O2 on their
 * own, and these operators are to stay one element at a time. They are the
 * baseline of the timing and the reference of the checks in
 * ops-selftest-simd and ops-selftest-cuda.
 * What each operator computes is in ops.h.
 *
 * Their memory is the memory of the program: upload() returns the bytes it
 * is given, download() copies.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ops.h"

static const char *init(void)
{
    return "CPU, 1 thread";
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

/* One row at a time, one weight at a time: decode it, multiply it, add
   it. Row r is the cols / 32 blocks of 18 bytes at w + r * row_bytes. */
static void matvec_q4_0(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols)
{
    size_t row_bytes = cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES;
    for (size_t r = 0; r < rows; r++) {
        const uint8_t *blk = w + r * row_bytes;
        float sum = 0;
        for (size_t c = 0; c < cols; c += 32, blk += 18) {
            float d = q4_0_scale(blk);
            for (int j = 0; j < 16; j++) {
                int lo = (blk[2 + j] & 15) - 8, hi = (blk[2 + j] >> 4) - 8;
                sum += d * lo * x[c + j] + d * hi * x[c + j + 16];
            }
        }
        y[r] = sum;
    }
}

static void embed(float *x, const uint8_t *blk, size_t n)
{
    for (size_t c = 0; c < n; c += 32, blk += 18) {
        float d = q4_0_scale(blk);
        for (int j = 0; j < 16; j++) {
            x[c + j]      = d * ((blk[2 + j] & 15) - 8);
            x[c + j + 16] = d * ((blk[2 + j] >> 4) - 8);
        }
    }
}

static void rmsnorm(float *y, const float *x, const float *gain, int n_vec, int d, float eps)
{
    for (int v = 0; v < n_vec; v++, x += d, y += d) {
        float ss = 0;
        for (int i = 0; i < d; i++)
            ss += x[i] * x[i];
        float scale = 1 / sqrtf(ss / d + eps);
        for (int i = 0; i < d; i++)
            y[i] = x[i] * scale * gain[i];
    }
}

static void rope(float *v, int n_vec, int d, int pos, float base)
{
    for (int h = 0; h < n_vec; h++, v += d)
        for (int i = 0; i < d / 2; i++) {
            float angle = pos * powf(base, -2.0f * i / d);
            float c = cosf(angle), s = sinf(angle);
            float a = v[i], b = v[i + d / 2];
            v[i]         = a * c - b * s;
            v[i + d / 2] = b * c + a * s;
        }
}

static float dot(const float *a, const float *b, int n)
{
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += a[i] * b[i];
    return sum;
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

static void softmax(float *s, int n_row, int n)
{
    for (int r = 0; r < n_row; r++, s += n) {
        float max = s[0], sum = 0;
        for (int i = 1; i < n; i++)
            if (s[i] > max)
                max = s[i];
        for (int i = 0; i < n; i++)
            sum += s[i] = expf(s[i] - max);
        for (int i = 0; i < n; i++)
            s[i] /= sum;
    }
}

static void attn_values(float *out, const float *s, const float *v_cache,
                        int n_head, int n_head_kv, int d, int n_tok)
{
    int kv = n_head_kv * d;
    for (int h = 0; h < n_head; h++) {
        int off = h / (n_head / n_head_kv) * d;
        float *o = out + h * d;
        for (int j = 0; j < d; j++)
            o[j] = 0;
        for (int t = 0; t < n_tok; t++)
            for (int j = 0; j < d; j++)
                o[j] += s[h * n_tok + t] * v_cache[(size_t)t * kv + off + j];
    }
}

static void swiglu(float *gate, const float *up, int n)
{
    for (int i = 0; i < n; i++)
        gate[i] = gate[i] / (1 + expf(-gate[i])) * up[i];
}

static void add(float *x, const float *y, int n)
{
    for (int i = 0; i < n; i++)
        x[i] += y[i];
}

static int argmax(const float *x, int n)
{
    int best = 0;
    for (int i = 1; i < n; i++)
        if (x[i] > x[best])
            best = i;
    return best;
}

const struct ops ops_scalar = {
    "scalar (ops/scalar.c)",
    init, alloc, upload, download, finish,
    matvec_q4_0, embed, rmsnorm, rope,
    attn_scores, softmax, attn_values, swiglu, add, argmax,
};
