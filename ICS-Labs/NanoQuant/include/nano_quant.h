/* nano_quant.h -- the functions you implement, and the helpers the framework
 * gives you for them.
 *
 * This header is the whole interface between your code and the framework:
 * the framework calls the functions declared here and nothing else in
 * impl/nano_quant.cpp, and your code needs nothing from the framework beyond
 * the helpers declared here. Write your code in impl/nano_quant.cpp, include
 * no framework header other than this one, and leave this file unchanged.
 *
 * Every function is pure. It reads its arguments, writes its outputs, and
 * does nothing else: no allocation, no printing, no exit. Multi-byte integers
 * are little endian. Floating-point arithmetic is done in float, in the order
 * the formulas below give it; the checks compare bytes, and a detour through
 * double changes the last bit of some results.
 */
#ifndef NANO_QUANT_H
#define NANO_QUANT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* ---------- Given by the framework ---------- */

/* Round to nearest, ties to even. Every "round" in part B below means this
 * function. (int)(x + 0.5f) differs from it on negative numbers and on ties.
 * The idiom: adding 2^23 + 2^22 moves the binary point to the last mantissa
 * bit, the hardware rounds the sum to nearest even, and the low bits of the
 * result are the integer. |f| must be below 2^22. */
static inline int nq_round(float f) {
    float v = f + 12582912.0f;
    int32_t i;
    memcpy(&i, &v, sizeof i);
    return (i & 0x007fffff) - 0x00400000;
}

/* Fit one Q4_K sub-block of 32 weights: find *scale >= 0 and *min >= 0 so that
 * x[i] is close to scale * q[i] - min for codes q[i] in 0..15. The fit is a
 * weighted least-squares search over 21 candidate scales, the same search as
 * make_qkx2_quants in ggml. It is implemented in src/q4_k_fit.cpp, and README.md,
 * part B, gives its formulas; q4_k_quantize calls it once per sub-block. */
void nq_q4_k_fit(const float *x, float *scale, float *min);

/* ---------- Part A: reading and assembling bits ---------- */

/* Assemble the 8 bytes at p, least significant first, into a 64-bit unsigned
 * integer. The framework reads the header length of a safetensors file with it. */
uint64_t rd_u64le(const uint8_t *p);

/* BF16 bits to float. BF16 has the sign and exponent fields of FP32, and its
 * 7 mantissa bits are the top 7 of the FP32 mantissa, so this is one shift. */
float bf16_to_f32(uint16_t h);

/* IEEE 754 half precision (1 sign, 5 exponent, 10 mantissa bits) to float.
 * Four kinds of input: normal, subnormal, zero, and infinity or NaN. */
float fp16_to_f32(uint16_t h);

/* float to IEEE 754 half precision, round to nearest, ties to even. Values
 * too large become infinity; values too small become subnormal or zero.
 * Every fp16 field in part B is written with this function. */
uint16_t f32_to_fp16(float f);

/* ---------- Part B: three block formats ----------
 *
 * Each quantize function turns one block of floats into the bytes of the
 * ggml format of the same name, and each dequantize function turns the bytes
 * back into floats. A block depends only on its own input. The fp16 fields
 * are written with f32_to_fp16 and read with fp16_to_f32, low byte first.
 * The formulas fix every rounding step, so one input has one correct output. */

#define NQ_Q4_0_BLOCK_ELEMS  32
#define NQ_Q4_0_BLOCK_BYTES  18   /* d:fp16, qs[16] */
#define NQ_Q4_1_BLOCK_ELEMS  32
#define NQ_Q4_1_BLOCK_BYTES  20   /* d:fp16, m:fp16, qs[16] */
#define NQ_Q4_K_BLOCK_ELEMS 256
#define NQ_Q4_K_BLOCK_BYTES 144   /* d:fp16, dmin:fp16, scales[12], qs[128] */

/* Q4_0: 32 weights, 18 bytes.
 *
 *   quantize    amax = the element with the largest |x[i]|, with its sign
 *                      (on a tie, the first one)
 *               d    = amax / -8.0f
 *               id   = d != 0 ? 1.0f / d : 0.0f
 *               q[i] = min(15, (int)(x[i] * id + 8.5f))
 *   dequantize  x[i] = d * (float)(q[i] - 8)
 *   bytes       blk[0..1] = d,  blk[2 + j] = q[j] | q[j + 16] << 4   (j = 0..15)
 *
 * The codes use the float d, before it is rounded to fp16. x[i] * id lies in
 * [-8, 8], so the cast truncates a positive number, which rounds x[i] * id + 8
 * half up; the min() catches the single code that would be 16. */
void q4_0_quantize(const float *x, uint8_t *blk);
void q4_0_dequantize(const uint8_t *blk, float *x);

/* Q4_1: 32 weights, 20 bytes.
 *
 *   quantize    lo, hi = the smallest and the largest x[i]
 *               d    = (hi - lo) / 15.0f
 *               id   = d != 0 ? 1.0f / d : 0.0f
 *               q[i] = min(15, (int)((x[i] - lo) * id + 0.5f))
 *               m    = lo
 *   dequantize  x[i] = d * (float)q[i] + m
 *   bytes       blk[0..1] = d,  blk[2..3] = m,  blk[4 + j] = q[j] | q[j + 16] << 4
 *
 * As in Q4_0, the codes use the float d and lo, before rounding to fp16. */
void q4_1_quantize(const float *x, uint8_t *blk);
void q4_1_dequantize(const uint8_t *blk, float *x);

/* Q4_K: 256 weights, 144 bytes, in 8 sub-blocks of 32. Sub-block j has a
 * 6-bit scale code sc[j] and a 6-bit offset code m[j]; the super-block stores
 * the two fp16 factors d and dmin that turn the codes back into floats.
 *
 *   quantize
 *     1. nq_q4_k_fit(x + 32*j, &s[j], &o[j])                  (j = 0..7)
 *        S = the largest s[j],  O = the largest o[j]
 *     2. d  = S / 63.0f,  dmin = O / 63.0f
 *        is = S > 0 ? 63.0f / S : 0.0f,  io = O > 0 ? 63.0f / O : 0.0f
 *        sc[j] = nq_round(is * s[j]),  m[j] = nq_round(io * o[j])   (both 0..63)
 *     3. D = fp16_to_f32(f32_to_fp16(d)) * (float)sc[j]
 *        M = fp16_to_f32(f32_to_fp16(dmin)) * (float)m[j]
 *        q[i] = D != 0 ? clamp(nq_round((x[i] + M) / D), 0, 15) : 0
 *   dequantize
 *        x[i] = (d * (float)sc[j]) * (float)q[i] - dmin * (float)m[j]
 *   bytes
 *     blk[0..1] = d,  blk[2..3] = dmin,
 *     blk[4..15] = the 8 pairs (sc[j], m[j]), written with put_scale_min,
 *     blk[16 + 32*g + l] = q[64*g + l] | q[64*g + 32 + l] << 4   (g = 0..3, l = 0..31)
 *
 * Step 3 uses d and dmin after the fp16 round trip, the values the
 * dequantizer reads back. Sub-block j is restored by the formula
 *     x = (d * sc[j]) * q - (dmin * m[j]). */
void q4_k_quantize(const float *x, uint8_t *blk);
void q4_k_dequantize(const uint8_t *blk, float *x);

/* The 12 scale bytes of Q4_K hold 8 pairs (sc, m) of 6 bits each.
 * get_scale_min reads pair j and is given in impl/nano_quant.cpp.
 * put_scale_min writes pair j: afterwards get_scale_min(j, q, ...) returns
 * sc and m, and every other bit of q keeps its value, so the 8 pairs can be
 * written in any order. sc and m are below 64. */
void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m);
void put_scale_min(int j, uint8_t *q, uint8_t sc, uint8_t m);

#endif /* NANO_QUANT_H */
