/* 助教的参考实现。接口与 student/student.cpp 完全相同，
   只包含 nano_quant.h，不用框架内部的任何东西——它就是一份正确的作业。
   判定用的参考产物由它生成。 */
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "nano_quant.h"

/* ---------- A ---------- */

uint64_t rd_u64le(const uint8_t *p) {
    uint64_t x = 0;
    for (int i = 7; i >= 0; i--) x = (x << 8) | (uint64_t)p[i];
    return x;
}

float bf16_to_f32(uint16_t h) {
    uint32_t bits = (uint32_t)h << 16;
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

float fp16_to_f32(uint16_t h) {
    uint32_t sign = (uint32_t)(h & 0x8000) << 16;
    uint32_t exp  = (uint32_t)(h >> 10) & 0x1f;
    uint32_t mant = (uint32_t)h & 0x3ff;
    uint32_t out;

    if (exp == 0) {
        if (mant == 0) {
            out = sign;
        } else {
            int e = -1;
            do { mant <<= 1; e++; } while (!(mant & 0x400));
            mant &= 0x3ff;
            out = sign | ((uint32_t)(127 - 15 - e) << 23) | (mant << 13);
        }
    } else if (exp == 0x1f) {
        out = sign | 0x7f800000u | (mant << 13);
    } else {
        out = sign | ((exp + 127 - 15) << 23) | (mant << 13);
    }
    float f;
    memcpy(&f, &out, 4);
    return f;
}

uint16_t f32_to_fp16(float f) {
    uint32_t x;
    memcpy(&x, &f, 4);

    uint32_t sign = (x >> 16) & 0x8000;
    uint32_t mant = x & 0x007fffff;
    int32_t  exp  = (int32_t)((x >> 23) & 0xff);

    if (exp == 0xff) return (uint16_t)(sign | (mant ? 0x7e00u : 0x7c00u));

    int32_t e = exp - 127 + 15;
    if (e >= 0x1f) return (uint16_t)(sign | 0x7c00u);

    if (e > 0) {
        uint32_t m = mant >> 13;
        uint32_t r = mant & 0x1fff;
        uint32_t h = sign | ((uint32_t)e << 10) | m;
        if (r > 0x1000 || (r == 0x1000 && (m & 1))) h++;
        return (uint16_t)h;
    }

    if (e < -10) return (uint16_t)sign;

    mant |= 0x00800000;
    int32_t  shift = 14 - e;
    uint32_t m     = mant >> shift;
    uint32_t r     = mant & ((1u << shift) - 1);
    uint32_t half  = 1u << (shift - 1);
    if (r > half || (r == half && (m & 1))) m++;
    return (uint16_t)(sign | m);
}

/* ---------- B ---------- */

void q4_0_quantize(const float *x, uint8_t *blk) {
    float amax = 0.0f, max = 0.0f;
    for (int j = 0; j < 32; j++) {
        float v = x[j];
        if (amax < fabsf(v)) { amax = fabsf(v); max = v; }
    }
    const float d  = max / -8.0f;
    const float id = d ? 1.0f / d : 0.0f;

    uint16_t dh = f32_to_fp16(d);
    blk[0] = (uint8_t)(dh & 0xff);
    blk[1] = (uint8_t)(dh >> 8);

    for (int j = 0; j < 16; j++) {
        const float x0 = x[j]      * id;
        const float x1 = x[j + 16] * id;
        const uint8_t q0 = (uint8_t)((int8_t)(x0 + 8.5f) < 15 ? (int8_t)(x0 + 8.5f) : 15);
        const uint8_t q1 = (uint8_t)((int8_t)(x1 + 8.5f) < 15 ? (int8_t)(x1 + 8.5f) : 15);
        blk[2 + j] = (uint8_t)(q0 | (q1 << 4));
    }
}

void q4_0_dequantize(const uint8_t *blk, float *x) {
    const float d = fp16_to_f32((uint16_t)(blk[0] | (blk[1] << 8)));
    for (int j = 0; j < 16; j++) {
        x[j]      = d * (float)((blk[2 + j] & 0xf) - 8);
        x[j + 16] = d * (float)((blk[2 + j] >>  4) - 8);
    }
}

void q4_1_quantize(const float *x, uint8_t *blk) {
    float min = FLT_MAX, max = -FLT_MAX;
    for (int j = 0; j < 32; j++) {
        if (x[j] < min) min = x[j];
        if (x[j] > max) max = x[j];
    }
    const float d  = (max - min) / 15.0f;
    const float id = d ? 1.0f / d : 0.0f;

    uint16_t dh = f32_to_fp16(d), mh = f32_to_fp16(min);
    blk[0] = (uint8_t)(dh & 0xff); blk[1] = (uint8_t)(dh >> 8);
    blk[2] = (uint8_t)(mh & 0xff); blk[3] = (uint8_t)(mh >> 8);

    for (int j = 0; j < 16; j++) {
        const float x0 = (x[j]      - min) * id;
        const float x1 = (x[j + 16] - min) * id;
        const uint8_t q0 = (uint8_t)((int8_t)(x0 + 0.5f) < 15 ? (int8_t)(x0 + 0.5f) : 15);
        const uint8_t q1 = (uint8_t)((int8_t)(x1 + 0.5f) < 15 ? (int8_t)(x1 + 0.5f) : 15);
        blk[4 + j] = (uint8_t)(q0 | (q1 << 4));
    }
}

void q4_1_dequantize(const uint8_t *blk, float *x) {
    const float d = fp16_to_f32((uint16_t)(blk[0] | (blk[1] << 8)));
    const float m = fp16_to_f32((uint16_t)(blk[2] | (blk[3] << 8)));
    for (int j = 0; j < 16; j++) {
        x[j]      = d * (float)(blk[4 + j] & 0xf) + m;
        x[j + 16] = d * (float)(blk[4 + j] >>  4) + m;
    }
}

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
    if (j < 4) {
        q[j]     = sc;
        q[j + 4] = m;
    } else {
        q[j + 4]  = (uint8_t)((sc & 0xf) | ((m & 0xf) << 4));
        q[j - 4] |= (uint8_t)((sc >> 4) << 6);
        q[j]     |= (uint8_t)((m  >> 4) << 6);
    }
}

/* 32 个元素的子块上找 (缩放, 偏移)。与 ggml 的 make_qkx2_quants 逐步相同：
   先用 [min, max] 的等分作为起点，再在 21 个略微收紧的候选缩放上做加权最小
   二乘，取加权平方误差最小的一组。README 有逐步说明。 */
static float fit_sub_block(const float *x, const float *w, uint8_t *L, float *the_min) {
    uint8_t Laux[32];
    float min = x[0], max = x[0];
    float sum_w = w[0], sum_x = w[0] * x[0];
    for (int i = 1; i < 32; ++i) {
        if (x[i] < min) min = x[i];
        if (x[i] > max) max = x[i];
        sum_w += w[i];
        sum_x += w[i] * x[i];
    }
    if (min > 0) min = 0;                 /* 偏移只往下取，保证 0 落在级上 */
    if (max == min) {
        for (int i = 0; i < 32; ++i) L[i] = 0;
        *the_min = -min;
        return 0.f;
    }

    float iscale = 15.0f / (max - min);
    float scale  = 1.0f / iscale;
    float best_error = 0;
    for (int i = 0; i < 32; ++i) {
        int l = nq_round(iscale * (x[i] - min));
        L[i] = (uint8_t)(l < 0 ? 0 : (l > 15 ? 15 : l));
        float diff = scale * (float)L[i] + min - x[i];
        best_error += w[i] * diff * diff;
    }

    for (int is = 0; is <= 20; ++is) {
        iscale = (-1.0f + 0.1f * (float)is + 15.0f) / (max - min);
        float sum_l = 0, sum_l2 = 0, sum_xl = 0;
        for (int i = 0; i < 32; ++i) {
            int l = nq_round(iscale * (x[i] - min));
            l = l < 0 ? 0 : (l > 15 ? 15 : l);
            Laux[i] = (uint8_t)l;
            sum_l  += w[i] * (float)l;
            sum_l2 += w[i] * (float)l * (float)l;
            sum_xl += w[i] * (float)l * x[i];
        }
        float D = sum_w * sum_l2 - sum_l * sum_l;
        if (D > 0) {
            float this_scale = (sum_w * sum_xl - sum_x * sum_l) / D;
            float this_min   = (sum_l2 * sum_x - sum_l * sum_xl) / D;
            if (this_min > 0) {
                this_min  = 0;
                this_scale = sum_xl / sum_l2;
            }
            float cur_error = 0;
            for (int i = 0; i < 32; ++i) {
                float diff = this_scale * (float)Laux[i] + this_min - x[i];
                cur_error += w[i] * diff * diff;
            }
            if (cur_error < best_error) {
                for (int i = 0; i < 32; ++i) L[i] = Laux[i];
                best_error = cur_error;
                scale = this_scale;
                min   = this_min;
            }
        }
    }
    *the_min = -min;
    return scale;
}

void q4_k_quantize(const float *x, uint8_t *blk) {
    uint8_t L[256];
    float   weights[32];
    float   mins[8], scales[8];

    float max_scale = 0, max_min = 0;
    for (int j = 0; j < 8; ++j) {
        float sum_x2 = 0;
        for (int l = 0; l < 32; ++l) sum_x2 += x[32*j + l] * x[32*j + l];
        float av_x = sqrtf(sum_x2 / 32.0f);
        for (int l = 0; l < 32; ++l) weights[l] = av_x + fabsf(x[32*j + l]);
        scales[j] = fit_sub_block(x + 32*j, weights, L + 32*j, &mins[j]);
        if (scales[j] > max_scale) max_scale = scales[j];
        if (mins[j]   > max_min)   max_min   = mins[j];
    }

    uint8_t *sc12 = blk + 4;
    memset(sc12, 0, 12);                  /* put_scale_min 用 |= 写高两位 */

    const float inv_scale = max_scale > 0 ? 63.0f / max_scale : 0.0f;
    const float inv_min   = max_min   > 0 ? 63.0f / max_min   : 0.0f;
    for (int j = 0; j < 8; ++j) {
        /* 先截成 8 位再与 63 取小，与 ggml 的写法一致 */
        uint8_t ls = (uint8_t)nq_round(inv_scale * scales[j]);
        uint8_t lm = (uint8_t)nq_round(inv_min   * mins[j]);
        if (ls > 63) ls = 63;
        if (lm > 63) lm = 63;
        put_scale_min(j, sc12, ls, lm);
    }

    const uint16_t dh  = f32_to_fp16(max_scale / 63.0f);
    const uint16_t dmh = f32_to_fp16(max_min   / 63.0f);
    blk[0] = (uint8_t)(dh  & 0xff); blk[1] = (uint8_t)(dh  >> 8);
    blk[2] = (uint8_t)(dmh & 0xff); blk[3] = (uint8_t)(dmh >> 8);

    for (int j = 0; j < 8; ++j) {
        uint8_t sc, m;
        get_scale_min(j, sc12, &sc, &m);
        const float d = fp16_to_f32(dh) * (float)sc;
        if (!d) continue;
        const float dm = fp16_to_f32(dmh) * (float)m;
        for (int ii = 0; ii < 32; ++ii) {
            int l = nq_round((x[32*j + ii] + dm) / d);
            L[32*j + ii] = (uint8_t)(l < 0 ? 0 : (l > 15 ? 15 : l));
        }
    }

    uint8_t *q = blk + 16;
    for (int j = 0; j < 256; j += 64) {
        for (int l = 0; l < 32; ++l) q[l] = (uint8_t)(L[j + l] | (L[j + l + 32] << 4));
        q += 32;
    }
}

void q4_k_dequantize(const uint8_t *blk, float *x) {
    const float d   = fp16_to_f32((uint16_t)(blk[0] | (blk[1] << 8)));
    const float min = fp16_to_f32((uint16_t)(blk[2] | (blk[3] << 8)));
    const uint8_t *sc12 = blk + 4;
    const uint8_t *q    = blk + 16;

    int is = 0;
    for (int j = 0; j < 256; j += 64) {
        uint8_t sc, m;
        get_scale_min(is + 0, sc12, &sc, &m);
        const float d1 = d * (float)sc, m1 = min * (float)m;
        get_scale_min(is + 1, sc12, &sc, &m);
        const float d2 = d * (float)sc, m2 = min * (float)m;
        for (int l = 0; l < 32; ++l) x[j + l]      = d1 * (float)(q[l] & 0xf) - m1;
        for (int l = 0; l < 32; ++l) x[j + l + 32] = d2 * (float)(q[l] >>  4) - m2;
        q += 32;
        is += 2;
    }
}

/* ---------- C ---------- */

bool tensor_selected(const char *name) {
    return strncmp(name, "model.language_model.", 21) == 0;
}

int layer_of(const char *name) {
    const char *p = strstr(name, ".layers.");
    if (!p) return -1;
    p += 8;
    if (*p < '0' || *p > '9') return -1;
    int n = 0;
    while (*p >= '0' && *p <= '9') n = n * 10 + (*p++ - '0');
    return n;
}

static bool ends_with(const char *s, const char *suf) {
    size_t a = strlen(s), b = strlen(suf);
    return a >= b && strcmp(s + a - b, suf) == 0;
}

/* llama.cpp 的 use_more_bits */
static bool more_bits(int i, int n) {
    return i < n / 8 || i >= 7 * n / 8 || (i - n / 8) % 3 == 2;
}

nq_type recipe_pick(nq_recipe r, const char *name, int n_dims, int layer, int n_layers) {
    if (n_dims == 1) return NQ_TYPE_F32;          /* 所有归一化系数留 F32 */

    switch (r) {
        case NQ_RECIPE_Q4_0: return NQ_TYPE_Q4_0;
        case NQ_RECIPE_Q4_1: return NQ_TYPE_Q4_1;
        case NQ_RECIPE_Q4_K: return NQ_TYPE_Q4_K;
        case NQ_RECIPE_Q4_K_M:
            if (ends_with(name, "embed_tokens.weight")) return NQ_TYPE_Q6_K;
            if (layer >= 0 && more_bits(layer, n_layers) &&
                (ends_with(name, "self_attn.v_proj.weight") ||
                 ends_with(name, "mlp.down_proj.weight"))) return NQ_TYPE_Q6_K;
            return NQ_TYPE_Q4_K;
    }
    return NQ_TYPE_Q4_K;
}

/* ---------- D ---------- */

int gguf_name(const char *hf_name, char *buf, size_t buf_size) {
    static const struct { const char *hf; const char *gg; } flat[] = {
        { "model.language_model.embed_tokens.weight", "token_embd.weight"  },
        { "model.language_model.norm.weight",         "output_norm.weight" },
    };
    for (size_t i = 0; i < sizeof flat / sizeof flat[0]; i++) {
        if (!strcmp(hf_name, flat[i].hf)) {
            if (strlen(flat[i].gg) + 1 > buf_size) return -1;
            strcpy(buf, flat[i].gg);
            return 0;
        }
    }

    static const struct { const char *suffix; const char *gg; } per_layer[] = {
        { "input_layernorm.weight",          "attn_norm"   },
        { "self_attn.q_proj.weight",         "attn_q"      },
        { "self_attn.k_proj.weight",         "attn_k"      },
        { "self_attn.v_proj.weight",         "attn_v"      },
        { "self_attn.o_proj.weight",         "attn_output" },
        { "self_attn.q_norm.weight",         "attn_q_norm" },
        { "self_attn.k_norm.weight",         "attn_k_norm" },
        { "post_attention_layernorm.weight", "ffn_norm"    },
        { "mlp.gate_proj.weight",            "ffn_gate"    },
        { "mlp.up_proj.weight",              "ffn_up"      },
        { "mlp.down_proj.weight",            "ffn_down"    },
    };

    const int layer = layer_of(hf_name);
    if (layer < 0) return -1;
    const char *p = strstr(hf_name, ".layers.");
    p += 8;
    while (*p >= '0' && *p <= '9') p++;
    if (*p != '.') return -1;
    p++;

    for (size_t i = 0; i < sizeof per_layer / sizeof per_layer[0]; i++) {
        if (!strcmp(p, per_layer[i].suffix)) {
            int n = snprintf(buf, buf_size, "blk.%d.%s.weight", layer, per_layer[i].gg);
            return (n > 0 && (size_t)n < buf_size) ? 0 : -1;
        }
    }
    return -1;
}
