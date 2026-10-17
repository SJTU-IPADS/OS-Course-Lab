/* ops.h -- the operators of mini-ollama, and the memory they work on:
 * every computation of the model is one call of a function in struct ops.
 *
 * Three sets of operators implement this table, one file each:
 *
 *   ops/scalar.c   ops_scalar   the operators of the lecture's mini_ollama,
 *                               one element at a time, on the CPU
 *   ops/simd.c     ops_simd     the same operators with AVX2 intrinsics,
 *                               8 floats per step, on the CPU
 *   ops/cuda.cu    ops_cuda     the same operators as CUDA kernels, on the
 *                               GPU
 *
 * A program is built with one set: mini-ollama-scalar, mini-ollama-simd
 * and mini-ollama-cuda are main.c with each of the three. Each set fills
 * the matvec entry of the table with its function matvec_q4_0. In
 * ops/simd.c everything is written except matvec_q4_0: it is yours in
 * step 1. In ops/cuda.cu everything is written except matvec_q4_0_kernel,
 * the kernel that matvec_q4_0 launches: it is yours in step 2. Keep this
 * file unchanged: the framework calls the operators only through the
 * table below.
 *
 * The operators of a set work on memory of that set: the two CPU sets on
 * the memory of the program, the GPU set on GPU memory only. The program
 * puts the weights there with upload(), makes room for the vectors with
 * alloc(), and reads a result back with download(). Every pointer an
 * operator takes is such memory.
 */
#ifndef OPS_H
#define OPS_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* q4_0_scale() is compiled for the CPU and, in ops/cuda.cu, also for the
 * GPU. */
#ifdef __CUDACC__
#define HOST_DEVICE __host__ __device__
#else
#define HOST_DEVICE
#endif

/* ---------- Q4_0, the format of every matrix ----------
 *
 * A row of n weights is n / 32 blocks of 18 bytes, one after another.
 * Block b, at blk = blocks + 18 * b, holds the weights w[32b .. 32b + 31]:
 *
 *   blk[0], blk[1]       the scale d of the block, read with q4_0_scale(blk)
 *   blk[2 + j], j=0..15  bits 3..0: the code q of weight w[32b + j]
 *                        bits 7..4: the code q of weight w[32b + 16 + j]
 *
 * A code q (0..15) stands for the weight d * (q - 8). */
#define Q4_0_BLOCK_WEIGHTS 32           /* weights in one block */
#define Q4_0_BLOCK_BYTES   18           /* d: fp16, then 16 bytes of codes */

/* The scale d of the block at blk: bytes blk[0] and blk[1] are a half-
 * precision number (1 sign bit, 5 exponent bits, 10 fraction bits), low byte
 * first. The result is that number as a float. */
static inline HOST_DEVICE float q4_0_scale(const uint8_t *blk)
{
    uint32_t h = blk[0] | (uint32_t)blk[1] << 8;
    uint32_t exp = h >> 10 & 31, frac = h & 1023, bits;
    float f;
    if (exp == 0) {                     /* zero or subnormal: frac * 2^-24 */
        f = (float)frac * (1.0f / 16777216.0f);
        memcpy(&bits, &f, sizeof bits);
    } else {                            /* normal: the float with the same value */
        bits = (exp + 112) << 23 | frac << 13;
    }
    bits |= (h & 0x8000u) << 16;        /* the sign, without a branch: it is
                                           random from block to block */
    memcpy(&f, &bits, sizeof f);
    return f;
}

/* ---------- The table ----------
 *
 * Vectors are arrays of float. Where an operator works on several vectors
 * at once (the heads of the attention), they lie one after another: vector
 * v of length d starts at element v * d. The caches hold one row per token:
 * the keys (or values) of token t are the n_head_kv * d floats at
 * t * n_head_kv * d, head after head. Query head h reads key head
 * h / (n_head / n_head_kv). */
struct ops {
    const char *name;                   /* printed by mini-ollama */

    /* ----- the machine and its memory ----- */

    /* Make the set ready; returns one line on the machine it runs on. */
    const char *(*init)(void);
    /* bytes of memory of this set, set to 0 */
    void *(*alloc)(size_t bytes);
    /* memory of this set holding a copy of the bytes at host; the CPU sets
     * return host itself */
    void *(*upload)(const void *host, size_t bytes);
    /* the bytes at mem copied to host */
    void (*download)(void *host, const void *mem, size_t bytes);
    /* returns when every operator called so far has finished: a kernel
     * launch returns before the kernel has run */
    void (*finish)(void);

    /* ----- the operators ----- */

    /* y = W x for a Q4_0 matrix W of rows x cols, stored row after row:
     * y[r] is the sum over c < cols of W[r][c] * x[c], computed in float.
     * cols is a multiple of 32. The order of the additions is free. */
    void (*matvec)(float *y, const uint8_t *w, const float *x, size_t rows, size_t cols);

    /* x = the n weights of one Q4_0 row, decoded. */
    void (*embed)(float *x, const uint8_t *row, size_t n);

    /* Each of the n_vec vectors of length d in x, scaled to root mean
     * square 1 and multiplied by the d gains: y = x / sqrt(mean(x^2) + eps)
     * * gain. y may be x. */
    void (*rmsnorm)(float *y, const float *x, const float *gain, int n_vec, int d, float eps);

    /* Each of the n_vec vectors of length d in v, marked with the position
     * pos: the pair (v[i], v[i + d/2]) turns by the angle
     * pos * base^(-2i/d), for i < d/2. */
    void (*rope)(float *v, int n_vec, int d, int pos, float base);

    /* s[h * n_tok + t] = (query h) . (key of token t, head h's key head)
     * / sqrt(d), for every query head h < n_head and token t < n_tok. q is
     * n_head vectors of length d. */
    void (*attn_scores)(float *s, const float *q, const float *k_cache,
                        int n_head, int n_head_kv, int d, int n_tok);

    /* Each of the n_row rows of n scores in s, turned into n weights that
     * are positive and add up to 1: s[i] = e^(s[i] - max) / sum. */
    void (*softmax)(float *s, int n_row, int n);

    /* out (n_head vectors of length d) = for each query head h, the sum
     * over t < n_tok of s[h * n_tok + t] times the value of token t (head
     * h's key head). */
    void (*attn_values)(float *out, const float *s, const float *v_cache,
                        int n_head, int n_head_kv, int d, int n_tok);

    /* gate[i] = silu(gate[i]) * up[i], with silu(a) = a / (1 + e^-a). */
    void (*swiglu)(float *gate, const float *up, int n);

    /* x[i] += y[i]. */
    void (*add)(float *x, const float *y, int n);

    /* The index of the largest of the n floats; the first one if several
     * are equal. The index is returned to the CPU. */
    int (*argmax)(const float *x, int n);
};

#ifdef __cplusplus
extern "C" {
#endif
extern const struct ops ops_scalar, ops_simd, ops_cuda;
#ifdef __cplusplus
}
#endif

#endif /* OPS_H */
