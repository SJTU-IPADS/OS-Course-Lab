/* ops.c -- the operators of mini-ollama, one element at a time, on one
 * thread of the CPU. What each operator computes is in ops.h.
 *
 * The Makefile compiles this file with -fno-tree-vectorize: GCC 12 and
 * later vectorize some loops at -O2 on their own, and these operators are
 * to stay one element at a time.
 */
#include <math.h>
#include <string.h>

#include "ops.h"

/* The scale of the Q4_0 block at block: bytes block[0] and block[1] are a
 * half-precision number (1 sign bit, 5 exponent bits, 10 fraction bits),
 * low byte first. The result is that number as a float. */
static inline float q4_0_scale(const uint8_t *block)
{
    uint32_t half = block[0] | (uint32_t)block[1] << 8;
    uint32_t exponent = half >> 10 & 31, fraction = half & 1023, bits;
    float value;
    if (exponent == 0) {                /* zero or subnormal: fraction * 2^-24 */
        value = (float)fraction * (1.0f / 16777216.0f);
        memcpy(&bits, &value, sizeof bits);
    } else {                            /* normal: the float with the same value */
        bits = (exponent + 112) << 23 | fraction << 13;
    }
    bits |= (half & 0x8000u) << 16;     /* the sign, without a branch: it is
                                           random from block to block */
    memcpy(&value, &bits, sizeof value);
    return value;
}

/* One row at a time, one weight at a time: decode it, multiply it, add
   it. Row number row is the cols / 32 blocks of 18 bytes at
   matrix + row * row_bytes. */
void matvec_q4_0(float *out, const uint8_t *matrix, const float *in, size_t rows, size_t cols)
{
    size_t row_bytes = cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES;
    for (size_t row = 0; row < rows; row++) {
        const uint8_t *block = matrix + row * row_bytes;
        float sum = 0;
        for (size_t col = 0; col < cols; col += 32, block += 18) {
            float scale = q4_0_scale(block);
            for (int i = 0; i < 16; i++) {
                int low = (block[2 + i] & 15) - 8, high = (block[2 + i] >> 4) - 8;
                sum += scale * low * in[col + i] + scale * high * in[col + i + 16];
            }
        }
        out[row] = sum;
    }
}

void embed_q4_0(float *out, const uint8_t *row, size_t count)
{
    const uint8_t *block = row;
    for (size_t col = 0; col < count; col += 32, block += 18) {
        float scale = q4_0_scale(block);
        for (int i = 0; i < 16; i++) {
            out[col + i]      = scale * ((block[2 + i] & 15) - 8);
            out[col + i + 16] = scale * ((block[2 + i] >> 4) - 8);
        }
    }
}

void rmsnorm(float *out, const float *in, const float *gain, int n_vec, int dim, float eps)
{
    for (int vec = 0; vec < n_vec; vec++, in += dim, out += dim) {
        float sum_sq = 0;
        for (int i = 0; i < dim; i++)
            sum_sq += in[i] * in[i];
        float scale = 1 / sqrtf(sum_sq / dim + eps);
        for (int i = 0; i < dim; i++)
            out[i] = in[i] * scale * gain[i];
    }
}

void rope(float *vec, int n_vec, int dim, int pos, float base)
{
    for (int head = 0; head < n_vec; head++, vec += dim)
        for (int i = 0; i < dim / 2; i++) {
            float angle = pos * powf(base, -2.0f * i / dim);
            float cos_angle = cosf(angle), sin_angle = sinf(angle);
            float first = vec[i], second = vec[i + dim / 2];
            vec[i]           = first * cos_angle - second * sin_angle;
            vec[i + dim / 2] = second * cos_angle + first * sin_angle;
        }
}

static float dot(const float *left, const float *right, int count)
{
    float sum = 0;
    for (int i = 0; i < count; i++)
        sum += left[i] * right[i];
    return sum;
}

void attn_scores(float *score, const float *query, float *const *keys,
                 int n_head, int n_head_kv, int head_dim, int n_tok)
{
    for (int head = 0; head < n_head; head++) {
        int offset = head / (n_head / n_head_kv) * head_dim;   /* its key head */
        for (int tok = 0; tok < n_tok; tok++)
            score[head * n_tok + tok] = dot(query + head * head_dim,
                                            keys[tok] + offset, head_dim)
                                        / sqrtf(head_dim);
    }
}

void softmax(float *score, int n_row, int count)
{
    for (int row = 0; row < n_row; row++, score += count) {
        float max = score[0], sum = 0;
        for (int i = 1; i < count; i++)
            if (score[i] > max)
                max = score[i];
        for (int i = 0; i < count; i++)
            sum += score[i] = expf(score[i] - max);
        for (int i = 0; i < count; i++)
            score[i] /= sum;
    }
}

void attn_values(float *out, const float *score, float *const *values,
                 int n_head, int n_head_kv, int head_dim, int n_tok)
{
    for (int head = 0; head < n_head; head++) {
        int offset = head / (n_head / n_head_kv) * head_dim;
        float *head_out = out + head * head_dim;
        for (int i = 0; i < head_dim; i++)
            head_out[i] = 0;
        for (int tok = 0; tok < n_tok; tok++)
            for (int i = 0; i < head_dim; i++)
                head_out[i] += score[head * n_tok + tok] * values[tok][offset + i];
    }
}

void swiglu(float *gate, const float *up, int count)
{
    for (int i = 0; i < count; i++)
        gate[i] = gate[i] / (1 + expf(-gate[i])) * up[i];
}

void add(float *acc, const float *in, int count)
{
    for (int i = 0; i < count; i++)
        acc[i] += in[i];
}

int argmax(const float *in, int count)
{
    int best = 0;
    for (int i = 1; i < count; i++)
        if (in[i] > in[best])
            best = i;
    return best;
}
