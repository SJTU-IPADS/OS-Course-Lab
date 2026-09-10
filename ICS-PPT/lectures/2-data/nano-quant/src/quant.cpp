/* 类型表与整行量化。学生写的是单块，成行的循环在这里。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nq.h"

namespace {
const nq_type_info INFO_F32  = { "F32",  1,   4 };
const nq_type_info INFO_F16  = { "F16",  1,   2 };
const nq_type_info INFO_Q4_0 = { "Q4_0", 32,  18 };
const nq_type_info INFO_Q4_1 = { "Q4_1", 32,  20 };
const nq_type_info INFO_Q4_K = { "Q4_K", 256, 144 };
const nq_type_info INFO_Q6_K = { "Q6_K", 256, 210 };
}

const nq_type_info *nq_type_of(nq_type t) {
    switch (t) {
        case NQ_TYPE_F32:  return &INFO_F32;
        case NQ_TYPE_F16:  return &INFO_F16;
        case NQ_TYPE_Q4_0: return &INFO_Q4_0;
        case NQ_TYPE_Q4_1: return &INFO_Q4_1;
        case NQ_TYPE_Q4_K: return &INFO_Q4_K;
        case NQ_TYPE_Q6_K: return &INFO_Q6_K;
    }
    return nullptr;
}

uint64_t nq_type_bytes(nq_type t, uint64_t n_elem) {
    const nq_type_info *ti = nq_type_of(t);
    return n_elem / (uint64_t)ti->block_elems * (uint64_t)ti->block_bytes;
}

void nq_quantize(nq_type t, const float *src, uint64_t n_elem, uint8_t *dst) {
    const nq_type_info *ti = nq_type_of(t);
    if (n_elem % (uint64_t)ti->block_elems != 0) {
        fprintf(stderr, "nano-quant: %llu 个元素不是 %s 块长 %d 的整数倍\n",
                (unsigned long long)n_elem, ti->name, ti->block_elems);
        exit(1);
    }
    if (t == NQ_TYPE_F32) { memcpy(dst, src, (size_t)n_elem * 4); return; }
    if (t == NQ_TYPE_F16) {
        uint16_t *h = (uint16_t *)dst;
        for (uint64_t i = 0; i < n_elem; i++) h[i] = nq_f32_to_fp16(src[i]);
        return;
    }

    const uint64_t nb = n_elem / (uint64_t)ti->block_elems;

    for (uint64_t b = 0; b < nb; b++) {
        const float *x = src + b * (uint64_t)ti->block_elems;
        uint8_t     *o = dst + b * (uint64_t)ti->block_bytes;
        switch (t) {
            case NQ_TYPE_F32:
            case NQ_TYPE_F16: break;
            case NQ_TYPE_Q4_0: q4_0_quantize(x, o);    break;
            case NQ_TYPE_Q4_1: q4_1_quantize(x, o);    break;
            case NQ_TYPE_Q4_K: q4_k_quantize(x, o);    break;
            case NQ_TYPE_Q6_K: nq_q6_k_quantize(x, o); break;
        }
    }
}

void nq_dequantize(nq_type t, const uint8_t *src, uint64_t n_elem, float *dst) {
    const nq_type_info *ti = nq_type_of(t);
    if (t == NQ_TYPE_F32) { memcpy(dst, src, (size_t)n_elem * 4); return; }
    if (t == NQ_TYPE_F16) {
        const uint16_t *h = (const uint16_t *)src;
        for (uint64_t i = 0; i < n_elem; i++) dst[i] = nq_fp16_to_f32(h[i]);
        return;
    }
    const uint64_t nb = n_elem / (uint64_t)ti->block_elems;
    for (uint64_t b = 0; b < nb; b++) {
        const uint8_t *i = src + b * (uint64_t)ti->block_bytes;
        float         *o = dst + b * (uint64_t)ti->block_elems;
        switch (t) {
            case NQ_TYPE_Q4_0: q4_0_dequantize(i, o);    break;
            case NQ_TYPE_Q4_1: q4_1_dequantize(i, o);    break;
            case NQ_TYPE_Q4_K: q4_k_dequantize(i, o);    break;
            case NQ_TYPE_Q6_K: nq_q6_k_dequantize(i, o); break;
            default: break;
        }
    }
}
