/* 助教提供的半精度与 BF16 转换。语义与学生 A 部分要写的三个函数相同，
   框架内部只用这一份，这样 A 部分还没写完时判定仍然跑得起来。 */
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
            out = sign;                       /* 零 */
        } else {                              /* 次规格化数：左移到隐含 1 出现 */
            int e = -1;
            do { mant <<= 1; e++; } while (!(mant & 0x400));
            mant &= 0x3ff;
            out = sign | ((uint32_t)(127 - 15 - e) << 23) | (mant << 13);
        }
    } else if (exp == 0x1f) {
        out = sign | 0x7f800000u | (mant << 13);   /* 无穷与 NaN */
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

    if (exp == 0xff) {                                   /* 无穷与 NaN */
        return (uint16_t)(sign | (mant ? 0x7e00u : 0x7c00u));
    }

    int32_t e = exp - 127 + 15;

    if (e >= 0x1f) return (uint16_t)(sign | 0x7c00u);    /* 上溢记成无穷 */

    if (e > 0) {                                         /* 规格化数 */
        uint32_t m = mant >> 13;
        uint32_t r = mant & 0x1fff;                      /* 舍去的 13 位 */
        uint32_t h = sign | ((uint32_t)e << 10) | m;
        if (r > 0x1000 || (r == 0x1000 && (m & 1))) h++; /* 就近舍入，平局取偶 */
        return (uint16_t)h;
    }

    if (e < -10) return (uint16_t)sign;                  /* 下溢记成零 */

    mant |= 0x00800000;                                  /* 补回隐含的 1 */
    int32_t  shift = 14 - e;                             /* 14 到 24 */
    uint32_t m     = mant >> shift;
    uint32_t r     = mant & ((1u << shift) - 1);
    uint32_t half  = 1u << (shift - 1);
    if (r > half || (r == half && (m & 1))) m++;
    return (uint16_t)(sign | m);
}
