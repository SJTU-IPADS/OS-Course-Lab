/* The framework's own half-precision and BF16 conversions: the same functions
   as the three of part A. The F16 and Q6_K formats use these, and nq-selftest
   compares yours against them; nq.h says which code uses which. */
#include <string.h>

#include "nq.h"

float nq_bf16_to_f32(uint16_t h) {
    uint32_t bits = (uint32_t)h << 16;
    float f;
    memcpy(&f, &bits, 4);
    return f;
}

float nq_fp16_to_f32(uint16_t h) {
    uint32_t sign = (uint32_t)(h & 0x8000) << 16;
    uint32_t exp  = (uint32_t)(h >> 10) & 0x1f;
    uint32_t mant = (uint32_t)h & 0x3ff;
    uint32_t out;

    if (exp == 0) {
        if (mant == 0) {
            out = sign;                       /* zero */
        } else {                              /* subnormal: shift until the hidden bit is 1 */
            int e = -1;
            do { mant <<= 1; e++; } while (!(mant & 0x400));
            mant &= 0x3ff;
            out = sign | ((uint32_t)(127 - 15 - e) << 23) | (mant << 13);
        }
    } else if (exp == 0x1f) {
        out = sign | 0x7f800000u | (mant << 13);   /* infinity and NaN */
    } else {
        out = sign | ((exp + 127 - 15) << 23) | (mant << 13);
    }

    float f;
    memcpy(&f, &out, 4);
    return f;
}

uint16_t nq_f32_to_fp16(float f) {
    uint32_t x;
    memcpy(&x, &f, 4);

    uint32_t sign = (x >> 16) & 0x8000;
    uint32_t mant = x & 0x007fffff;
    int32_t  exp  = (int32_t)((x >> 23) & 0xff);

    if (exp == 0xff) {                                   /* infinity and NaN */
        return (uint16_t)(sign | (mant ? 0x7e00u : 0x7c00u));
    }

    int32_t e = exp - 127 + 15;

    if (e >= 0x1f) return (uint16_t)(sign | 0x7c00u);    /* overflow: infinity */

    if (e > 0) {                                         /* normal */
        uint32_t m = mant >> 13;
        uint32_t r = mant & 0x1fff;                      /* the 13 bits dropped */
        uint32_t h = sign | ((uint32_t)e << 10) | m;
        if (r > 0x1000 || (r == 0x1000 && (m & 1))) h++; /* nearest, ties to even */
        return (uint16_t)h;
    }

    if (e < -10) return (uint16_t)sign;                  /* underflow: zero */

    mant |= 0x00800000;                                  /* restore the hidden 1 */
    int32_t  shift = 14 - e;                             /* 14 to 24 */
    uint32_t m     = mant >> shift;
    uint32_t r     = mant & ((1u << shift) - 1);
    uint32_t half  = 1u << (shift - 1);
    if (r > half || (r == half && (m & 1))) m++;
    return (uint16_t)(sign | m);
}
