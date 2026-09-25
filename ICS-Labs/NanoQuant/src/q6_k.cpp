/* Q6_K, given by the framework.
 *
 * 256 elements per super-block, in 16 groups of 16, each group with a signed
 * 8-bit scale; the super-block stores one fp16 d. Codes have 6 bits: the low 4
 * in ql, the high 2 in qh. 210 bytes / 256 weights = 6.5625 bits.
 *
 * The layout is ggml's block_q6_K: ql[128], qh[64], scales[16], d (fp16).
 * You do not implement Q6_K; the q4_k_m recipe uses it for the embeddings
 * and some layers. */
#include <math.h>
#include <string.h>

#include "nq.h"

namespace {

/* The common scale for codes in [-nmax, nmax-1] with the least weighted
   squared error: make_qx_quants from ggml with rmse_type = 1, step by step. */
float make_qx_quants(int n, int nmax, const float *x, int8_t *L) {
    float max = 0, amax = 0;
    for (int i = 0; i < n; ++i) {
        float ax = fabsf(x[i]);
        if (ax > amax) { amax = ax; max = x[i]; }
    }
    if (amax < 1e-15f) {
        for (int i = 0; i < n; ++i) L[i] = 0;
        return 0.f;
    }
    float iscale = -(float)nmax / max;
    float sumlx = 0, suml2 = 0;
    for (int i = 0; i < n; ++i) {
        int l = nq_round(iscale * x[i]);
        if (l < -nmax)   l = -nmax;
        if (l > nmax - 1) l = nmax - 1;
        L[i] = (int8_t)(l + nmax);
        float w = x[i] * x[i];
        sumlx += w * x[i] * (float)l;
        suml2 += w * (float)l * (float)l;
    }
    float scale = suml2 ? sumlx / suml2 : 0.0f;
    float best  = scale * sumlx;
    for (int is = -9; is <= 9; ++is) {
        if (is == 0) continue;
        iscale = -((float)nmax + 0.1f * (float)is) / max;
        sumlx = suml2 = 0;
        for (int i = 0; i < n; ++i) {
            int l = nq_round(iscale * x[i]);
            if (l < -nmax)   l = -nmax;
            if (l > nmax - 1) l = nmax - 1;
            float w = x[i] * x[i];
            sumlx += w * x[i] * (float)l;
            suml2 += w * (float)l * (float)l;
        }
        if (suml2 > 0 && sumlx * sumlx > best * suml2) {
            for (int i = 0; i < n; ++i) {
                int l = nq_round(iscale * x[i]);
                if (l < -nmax)   l = -nmax;
                if (l > nmax - 1) l = nmax - 1;
                L[i] = (int8_t)(l + nmax);
            }
            scale = sumlx / suml2;
            best  = scale * sumlx;
        }
    }
    return scale;
}

struct blk_q6_k {
    uint8_t ql[128];
    uint8_t qh[64];
    int8_t  scales[16];
    uint16_t d;
};
static_assert(sizeof(blk_q6_k) == NQ_Q6_K_BLOCK_BYTES, "block_q6_K must be 210 bytes");

} /* namespace */

void nq_q6_k_quantize(const float *x, uint8_t *out) {
    blk_q6_k y;
    memset(&y, 0, sizeof y);

    int8_t L[256];
    float  scales[16];
    float  max_scale = 0, max_abs_scale = 0;

    for (int ib = 0; ib < 16; ++ib) {
        float scale = make_qx_quants(16, 32, x + 16 * ib, L + 16 * ib);
        scales[ib] = scale;
        float a = fabsf(scale);
        if (a > max_abs_scale) { max_abs_scale = a; max_scale = scale; }
    }

    if (max_abs_scale < 1e-15f) {
        memcpy(out, &y, sizeof y);
        return;
    }

    float iscale = -128.f / max_scale;
    y.d = nq_f32_to_fp16(1 / iscale);
    for (int ib = 0; ib < 16; ++ib) {
        int v = nq_round(iscale * scales[ib]);
        y.scales[ib] = (int8_t)(v < 127 ? v : 127);
    }

    for (int j = 0; j < 16; ++j) {
        float d = nq_fp16_to_f32(y.d) * (float)y.scales[j];
        if (!d) continue;
        for (int ii = 0; ii < 16; ++ii) {
            int l = nq_round(x[16 * j + ii] / d);
            if (l < -32) l = -32;
            if (l >  31) l =  31;
            L[16 * j + ii] = (int8_t)(l + 32);
        }
    }

    uint8_t *ql = y.ql;
    uint8_t *qh = y.qh;
    for (int j = 0; j < 256; j += 128) {
        for (int l = 0; l < 32; ++l) {
            const uint8_t q1 = (uint8_t)L[j + l +  0] & 0xF;
            const uint8_t q2 = (uint8_t)L[j + l + 32] & 0xF;
            const uint8_t q3 = (uint8_t)L[j + l + 64] & 0xF;
            const uint8_t q4 = (uint8_t)L[j + l + 96] & 0xF;
            ql[l +  0] = (uint8_t)(q1 | (q3 << 4));
            ql[l + 32] = (uint8_t)(q2 | (q4 << 4));
            qh[l] = (uint8_t)(((uint8_t)L[j + l] >> 4)
                            | (((uint8_t)L[j + l + 32] >> 4) << 2)
                            | (((uint8_t)L[j + l + 64] >> 4) << 4)
                            | (((uint8_t)L[j + l + 96] >> 4) << 6));
        }
        ql += 64;
        qh += 32;
    }

    memcpy(out, &y, sizeof y);
}

void nq_q6_k_dequantize(const uint8_t *in, float *y) {
    blk_q6_k x;
    memcpy(&x, in, sizeof x);

    const float d = nq_fp16_to_f32(x.d);
    const uint8_t *ql = x.ql;
    const uint8_t *qh = x.qh;
    const int8_t  *sc = x.scales;

    for (int n = 0; n < 256; n += 128) {
        for (int l = 0; l < 32; ++l) {
            int is = l / 16;
            const int8_t q1 = (int8_t)((ql[l +  0] & 0xF) | (((qh[l] >> 0) & 3) << 4)) - 32;
            const int8_t q2 = (int8_t)((ql[l + 32] & 0xF) | (((qh[l] >> 2) & 3) << 4)) - 32;
            const int8_t q3 = (int8_t)((ql[l +  0] >>  4) | (((qh[l] >> 4) & 3) << 4)) - 32;
            const int8_t q4 = (int8_t)((ql[l + 32] >>  4) | (((qh[l] >> 6) & 3) << 4)) - 32;
            y[l +  0] = d * (float)sc[is + 0] * (float)q1;
            y[l + 32] = d * (float)sc[is + 2] * (float)q2;
            y[l + 64] = d * (float)sc[is + 4] * (float)q3;
            y[l + 96] = d * (float)sc[is + 6] * (float)q4;
        }
        y  += 128;
        ql += 64;
        qh += 32;
        sc += 8;
    }
}
