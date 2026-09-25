/* nano_quant.cpp -- all the code you write goes in this file.
 *
 * Replace the todo() call in each function with an implementation. What each
 * function must compute is in include/nano_quant.h. Keep the prototypes as
 * they are and add nothing to the header: the framework only calls what the
 * header declares.
 *
 *   make                    build nano-quant, nq2gguf and nq-selftest
 *   ./nq-selftest a q4_0    check parts A and B, group by group
 *   make test               build, make the test files, run tests/run.sh
 *
 * Do the arithmetic in float, never through double, and keep the compiler
 * flags in the Makefile.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nano_quant.h"

static void todo(const char *who) {
    fprintf(stderr, "nano_quant.cpp: %s is not implemented yet\n", who);
    exit(3);
}

/* ================= Part A: reading and assembling bits ================= */

uint64_t rd_u64le(const uint8_t *p) {
    (void)p;
    todo("rd_u64le");
    return 0;
}

float bf16_to_f32(uint16_t h) {
    (void)h;
    todo("bf16_to_f32");
    return 0;
}

float fp16_to_f32(uint16_t h) {
    (void)h;
    todo("fp16_to_f32");
    return 0;
}

uint16_t f32_to_fp16(float f) {
    (void)f;
    todo("f32_to_fp16");
    return 0;
}

/* ================= Part B: three block formats ================= */

void q4_0_quantize(const float *x, uint8_t *blk) {
    (void)x; (void)blk;
    todo("q4_0_quantize");
}

void q4_0_dequantize(const uint8_t *blk, float *x) {
    (void)blk; (void)x;
    todo("q4_0_dequantize");
}

void q4_1_quantize(const float *x, uint8_t *blk) {
    (void)x; (void)blk;
    todo("q4_1_quantize");
}

void q4_1_dequantize(const uint8_t *blk, float *x) {
    (void)blk; (void)x;
    todo("q4_1_dequantize");
}

/* Given. Write put_scale_min so that this function reads back what it wrote. */
void get_scale_min(int j, const uint8_t *q, uint8_t *sc, uint8_t *m) {
    if (j < 4) {
        *sc = q[j] & 63;
        *m  = q[j + 4] & 63;
    } else {
        *sc = (uint8_t)((q[j + 4] & 0xf) | ((q[j - 4] >> 6) << 4));
        *m  = (uint8_t)((q[j + 4] >>  4) | ((q[j    ] >> 6) << 4));
    }
}

void put_scale_min(int j, uint8_t *q, uint8_t sc, uint8_t m) {
    (void)j; (void)q; (void)sc; (void)m;
    todo("put_scale_min");
}

void q4_k_quantize(const float *x, uint8_t *blk) {
    (void)x; (void)blk;
    todo("q4_k_quantize");
}

void q4_k_dequantize(const uint8_t *blk, float *x) {
    (void)blk; (void)x;
    todo("q4_k_dequantize");
}
