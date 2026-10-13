/* math.c -- the single-precision functions mini-ollama calls.

   The machine adds, subtracts, multiplies and divides floats, takes their
   square root and converts between float and int. Every function here is
   made of those operations, with a union to reach the bits of a float; no
   double appears. Each one brings its argument into a short interval and
   adds up a few terms of a series there. expf, logf, sinf and cosf are
   within 3 units in the last place of the exact values; powf is less
   exact, as its comment says. */
#include "libc.h"

union bits {
    float    f;
    uint32_t u;                         /* 1 sign bit, 8 exponent bits, 23 fraction bits */
};

static float from_bits(uint32_t u)
{
    union bits b = { .u = u };
    return b.f;
}

static uint32_t to_bits(float f)
{
    union bits b = { .f = f };
    return b.u;
}

static const uint32_t INF = 0x7f800000, NOT_A_NUMBER = 0x7fc00000;

/* v rounded to the nearest integer. */
static int nearest(float v)
{
    int n = (int)v;                     /* the fraction is cut off */
    float rest = v - (float)n;
    if (rest > 0.5f)
        n++;
    if (rest < -0.5f)
        n--;
    return n;
}

/* One instruction, sqrtss. */
float sqrtf(float x)
{
    return __builtin_sqrtf(x);
}

/* ---------------- powers of 2 ---------------- */

/* x * 2^e. For e from -126 to 127 the float 2^e has the exponent field
   e + 127 and a fraction of 0, and the result is one multiplication. A
   factor outside that range is applied in steps. A product below 2^-126
   keeps fewer bits and is rounded; on the way down the steps are 2^-102,
   which leaves that rounding to the last multiplication. */
float ldexpf(float x, int e)
{
    if (e > 512)                        /* far beyond the range of a float */
        e = 512;
    if (e < -512)
        e = -512;
    for (; e > 127; e -= 127)
        x *= 0x1p127f;
    for (; e < -126; e += 102)
        x *= 0x1p-102f;
    return x * from_bits((uint32_t)(e + 127) << 23);
}

/* ---------------- exponential and logarithm ---------------- */

/* ln 2 in two parts. LN2_HI has 16 significant bits, so its product with
   an integer below 256 is exact. */
static const float LN2_HI = 0.693145751953125f;
static const float LN2_LO = 1.42860682e-6f;
static const float INV_LN2 = 1.44269504f;

/* e^x. With k = x / ln 2 rounded to an integer and r = x - k ln 2,
   e^x = 2^k e^r and |r| <= 0.347. There the series
   1 + r + r^2/2! + ... + r^8/8! leaves out less than 2e-10. r is kept as
   hi - lo, where hi is exact, and enters the sum in those two parts. */
float expf(float x)
{
    if (x != x)
        return x;                       /* not a number */
    if (x > 89)
        return from_bits(INF);          /* above the largest float */
    if (x < -104)
        return 0;                       /* below half the smallest float */

    int k = nearest(x * INV_LN2);
    float hi = x - (float)k * LN2_HI, lo = (float)k * LN2_LO;
    float r = hi - lo;
    float p = r * r * (1.0f / 2 + r * (1.0f / 6 + r * (1.0f / 24 + r * (1.0f / 120 +
              r * (1.0f / 720 + r * (1.0f / 5040 + r * (1.0f / 40320)))))));
    return ldexpf(1 + (hi + (p - lo)), k);
}

/* ln x. x = 2^k m with m between 0.71 and 1.42, so ln x = k ln 2 + ln m.
   With f = m - 1 and s = f / (2 + f), |s| < 0.172 and
   ln m = 2 (s + s^3/3 + s^5/5 + s^7/7 + s^9/9), less than 7e-10 left out.
   Since 2 s = f - s f, the sum is f - s (f - p) with
   p = 2 (s^2/3 + s^4/5 + s^6/7 + s^8/9): its largest term, f, is exact. */
float logf(float x)
{
    uint32_t u = to_bits(x);
    int k = 0;

    if (x != x || u == INF)
        return x;                       /* not a number, +infinity */
    if (x == 0)
        return from_bits(INF | 1u << 31);
    if (x < 0)
        return from_bits(NOT_A_NUMBER);
    if (u < 1u << 23) {                 /* below 2^-126: no leading 1 is stored */
        u = to_bits(x * 0x1p25f);
        k = -25;
    }
    k += (int)(u >> 23) - 127;
    float m = from_bits((u & 0x007fffff) | 0x3f800000);     /* 1 <= m < 2 */
    if (m > 1.41421356f) {
        m *= 0.5f;
        k++;
    }

    float f = m - 1, s = f / (2 + f), z = s * s;
    float p = z * (2.0f / 3 + z * (2.0f / 5 + z * (2.0f / 7 + z * (2.0f / 9))));
    return (float)k * LN2_HI + (f + ((float)k * LN2_LO - s * (f - p)));
}

/* x^y = e^(y ln x), for x >= 0. The product y ln x is rounded before the
   exponential, so a result far from 1 is less exact than expf and logf
   are: the error grows with |y ln x|. */
float powf(float x, float y)
{
    if (y == 0)
        return 1;
    return expf(y * logf(x));
}

/* ---------------- sine and cosine ---------------- */

/* pi/2 in four parts, written in hexadecimal: 1.921c is 1 + 0x921c / 65536.
   The first three have 15 significant bits each, so their products with an
   integer below 512 are exact. */
static const float PIO2_1 = 0x1.921cp+0f;       /* 1.5707397 */
static const float PIO2_2 = 0x1.daap-15f;       /* 5.6579709e-5 */
static const float PIO2_3 = 0x1.10b4p-30f;      /* 9.9208819e-10 */
static const float PIO2_4 = 0x1.84698ap-48f;    /* 5.3903029e-15 */
static const float INV_PIO2 = 0.636619747f;

/* The angle x as n quarter turns and a rest: x = n pi/2 + *r with
   |*r| <= pi/4. n is x / (pi/2) rounded to an integer. The four parts of
   n pi/2 are taken off one after another: when x is close to n pi/2, the
   first three differences are exact and the rest keeps all its digits.
   This holds while n is below 512, that is for |x| below 800. */
static int quarter_turns(float x, float *r)
{
    int n = nearest(x * INV_PIO2);
    float k = (float)n;
    *r = (((x - k * PIO2_1) - k * PIO2_2) - k * PIO2_3) - k * PIO2_4;
    return n;
}

/* sin r for |r| <= pi/4: r - r^3/3! + r^5/5! - r^7/7! + r^9/9!, less
   than 2e-9 left out. */
static float sin_series(float r)
{
    float z = r * r;
    return r + r * z * (-1.0f / 6 + z * (1.0f / 120 + z * (-1.0f / 5040 + z * (1.0f / 362880))));
}

/* cos r for |r| <= pi/4: 1 - r^2/2! + r^4/4! - ... - r^10/10!, less than
   2e-10 left out. */
static float cos_series(float r)
{
    float z = r * r;
    return 1 - 0.5f * z + z * z * (1.0f / 24 + z * (-1.0f / 720 + z * (1.0f / 40320 +
           z * (-1.0f / 3628800))));
}

/* The sine of n pi/2 + r: one of sin r, cos r, -sin r, -cos r. */
static float sin_turned(int n, float r)
{
    float v = n & 1 ? cos_series(r) : sin_series(r);
    return n & 2 ? -v : v;
}

float sinf(float x)
{
    float r;
    int n = quarter_turns(x, &r);
    return sin_turned(n, r);
}

/* cos x = sin(x + pi/2): one quarter turn more. */
float cosf(float x)
{
    float r;
    int n = quarter_turns(x, &r);
    return sin_turned(n + 1, r);
}
