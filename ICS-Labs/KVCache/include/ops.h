/* ops.h -- the operators of mini-ollama: every computation of the model is
 * one call of a function below. frame/ops.c computes them one element at a
 * time on one thread of the CPU, and the Makefile compiles it with
 * -fno-tree-vectorize, so that the compiler keeps it that way.
 *
 * Vectors are arrays of float. Where an operator works on several vectors
 * at once (the heads of the attention), they lie one after another: vector
 * number idx of length dim starts at element idx * dim.
 *
 * The keys (and the values) of the attention are rows, one per token, each
 * given by its address: keys[tok] points to the key of token tok, which is
 * n_head_kv * head_dim floats, head after head. The rows may lie anywhere.
 * Query head number head reads key head number head / (n_head / n_head_kv).
 */
#ifndef OPS_H
#define OPS_H

#include <stddef.h>
#include <stdint.h>

/* ---------- Q4_0, the format of every matrix ----------
 *
 * A row of count weights is count / 32 blocks of 18 bytes, one after
 * another. A block holds 32 weights: its first two bytes are the scale of
 * the block, a half-precision number, low byte first; byte 2 + idx, for
 * idx < 16, holds the code of weight idx in its bits 3..0 and the code of
 * weight 16 + idx in its bits 7..4. A code (0..15) stands for the weight
 * scale * (code - 8). */
#define Q4_0_BLOCK_WEIGHTS 32           /* weights in one block */
#define Q4_0_BLOCK_BYTES   18           /* the scale, then 16 bytes of codes */

/* out = the matrix times in, for a Q4_0 matrix of rows x cols stored row
 * after row: out[row] is the sum over col < cols of weight[row][col] *
 * in[col], computed in float. cols is a multiple of 32. */
void matvec_q4_0(float *out, const uint8_t *matrix, const float *in, size_t rows, size_t cols);

/* out = the count weights of one Q4_0 row, decoded. */
void embed_q4_0(float *out, const uint8_t *row, size_t count);

/* Each of the n_vec vectors of length dim in in, scaled to root mean square
 * 1 and multiplied by the dim gains: out = in / sqrt(mean(in^2) + eps) *
 * gain. out may be in. */
void rmsnorm(float *out, const float *in, const float *gain, int n_vec, int dim, float eps);

/* Each of the n_vec vectors of length dim in vec, marked with the position
 * pos: the pair (vec[i], vec[i + dim/2]) turns by the angle
 * pos * base^(-2i/dim), for i < dim/2. */
void rope(float *vec, int n_vec, int dim, int pos, float base);

/* score[head * n_tok + tok] = (query head) . (key of token tok, the key
 * head of that query head) / sqrt(head_dim), for every query head
 * head < n_head and token tok < n_tok. query is n_head vectors of length
 * head_dim, keys holds the addresses of n_tok rows. */
void attn_scores(float *score, const float *query, float *const *keys,
                 int n_head, int n_head_kv, int head_dim, int n_tok);

/* Each of the n_row rows of count scores in score, turned into count
 * weights that are positive and add up to 1:
 * score[i] = e^(score[i] - max) / sum. */
void softmax(float *score, int n_row, int count);

/* out (n_head vectors of length head_dim) = for each query head, the sum
 * over tok < n_tok of score[head * n_tok + tok] times the value of token
 * tok (the key head of that query head). values holds the addresses of
 * n_tok rows. */
void attn_values(float *out, const float *score, float *const *values,
                 int n_head, int n_head_kv, int head_dim, int n_tok);

/* gate[i] = silu(gate[i]) * up[i], with silu(val) = val / (1 + e^-val). */
void swiglu(float *gate, const float *up, int count);

/* acc[i] += in[i]. */
void add(float *acc, const float *in, int count);

/* The index of the largest of the count floats; the first one if several
 * are equal. */
int argmax(const float *in, int count);

#endif /* OPS_H */
