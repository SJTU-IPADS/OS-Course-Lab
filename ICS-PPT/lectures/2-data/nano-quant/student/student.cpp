/* student.cpp —— 你要写的全部代码都在这里。
 *
 * 每个函数下面的 TODO 换成你的实现。函数原型不要改，也不要往
 * nano_quant.h 里加东西：判定程序只按那个头文件调用。
 *
 * 编译：make
 * 自查：./nq-selftest              （A、B 两部分，不需要模型文件）
 *       ./nano-quant plan <model.safetensors>
 *       ./nano-quant quant <model.safetensors> --recipe q4_k_m -o out.nq
 *
 * 一条纪律：全程用 float，不要用 double 中转，也不要改 Makefile 的编译选项。
 * 判定要求逐字节相同，这两件事上任何偏离都会让结果对不上。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nano_quant.h"

static void todo(const char *who) {
    fprintf(stderr, "student.cpp: %s 还没有实现\n", who);
    exit(3);
}

/* ================= A 部分：读位与拼位 ================= */

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

/* ================= B 部分：三种块格式 ================= */

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

/* 这一个已经给出，照它写出 put_scale_min。 */
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

/* ================= C 部分：按配方分派 ================= */

bool tensor_selected(const char *name) {
    (void)name;
    todo("tensor_selected");
    return false;
}

nq_type recipe_pick(nq_recipe r, const char *name, int n_dims, int layer, int n_layers) {
    (void)r; (void)name; (void)n_dims; (void)layer; (void)n_layers;
    todo("recipe_pick");
    return NQ_TYPE_F32;
}

/* ================= D 部分：名字映射 ================= */

int layer_of(const char *name) {
    (void)name;
    todo("layer_of");
    return -1;
}

int gguf_name(const char *hf_name, char *buf, size_t buf_size) {
    (void)hf_name; (void)buf; (void)buf_size;
    todo("gguf_name");
    return -1;
}
