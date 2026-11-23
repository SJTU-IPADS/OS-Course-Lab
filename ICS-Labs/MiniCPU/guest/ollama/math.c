/* math.c -- the single-precision functions mini-ollama calls.

   The machine adds, subtracts, multiplies and divides floats, takes their
   square root and converts between float and int. Every function here is
   made of those operations, with a union to reach the bits of a float; no
   double appears. Each one brings its argument into a short interval and
   adds up a few terms of a series there. expf, logf, sinf and cosf are
   within 3 units in the last place of the exact values; powf is less
   exact, as its comment says. */
#include "libc.h"

union float_bits {
    float    value;
    uint32_t bits;                      /* 1 sign bit, 8 exponent bits, 23 fraction bits */
};

static float from_bits(uint32_t bits)
{
    union float_bits number = { .bits = bits };
    return number.value;
}

static uint32_t to_bits(float value)
{
    union float_bits number = { .value = value };
    return number.bits;
}

static const uint32_t INF = 0x7f800000, NOT_A_NUMBER = 0x7fc00000;

/* value rounded to the nearest integer. */
static int nearest(float value)
{
    int whole = (int)value;             /* the fraction is cut off */
    float rest = value - (float)whole;
    if (rest > 0.5f)
        whole++;
    if (rest < -0.5f)
        whole--;
    return whole;
}

/* One instruction, sqrtss. */
float sqrtf(float value)
{
    return __builtin_sqrtf(value);
}

/* ---------------- powers of 2 ---------------- */

/* x * 2^e for x = value, e = exponent. For e from -126 to 127 the float 2^e
   has the exponent field e + 127 and a fraction of 0, and the result is one
   multiplication. A factor outside that range is applied in steps. A
   product below 2^-126 keeps fewer bits and is rounded; on the way down the
   steps are 2^-102, which leaves that rounding to the last multiplication. */
float ldexpf(float value, int exponent)
{
    if (exponent > 512)                 /* far beyond the range of a float */
        exponent = 512;
    if (exponent < -512)
        exponent = -512;
    for (; exponent > 127; exponent -= 127)
        value *= 0x1p127f;
    for (; exponent < -126; exponent += 102)
        value *= 0x1p-102f;
    return value * from_bits((uint32_t)(exponent + 127) << 23);
}

/* ---------------- exponential and logarithm ---------------- */

/* ln 2 in two parts. LN2_HI has 16 significant bits, so its product with
   an integer below 256 is exact. */
static const float LN2_HI = 0.693145751953125f;
static const float LN2_LO = 1.42860682e-6f;
static const float INV_LN2 = 1.44269504f;

/* e^x for x = value. With k = x / ln 2 rounded to an integer (exponent) and
   r = x - k ln 2 (rest), e^x = 2^k e^r and |r| <= 0.347. There the series
   1 + r + r^2/2! + ... + r^8/8! leaves out less than 2e-10; poly is its part
   from r^2/2! on. r is kept as hi - lo, where hi is exact, and enters the
   sum in those two parts. */
float expf(float value)
{
    if (value != value)
        return value;                   /* not a number */
    if (value > 89)
        return from_bits(INF);          /* above the largest float */
    if (value < -104)
        return 0;                       /* below half the smallest float */

    int exponent = nearest(value * INV_LN2);
    float hi = value - (float)exponent * LN2_HI, lo = (float)exponent * LN2_LO;
    float rest = hi - lo;
    float poly = rest * rest * (1.0f / 2 + rest * (1.0f / 6 + rest * (1.0f / 24 +
                 rest * (1.0f / 120 + rest * (1.0f / 720 + rest * (1.0f / 5040 +
                 rest * (1.0f / 40320)))))));
    return ldexpf(1 + (hi + (poly - lo)), exponent);
}

/* ln x for x = value. x = 2^k m with m between 0.71 and 1.42 (k is exponent,
   m mantissa), so ln x = k ln 2 + ln m. With f = m - 1 (delta) and
   s = f / (2 + f) (ratio), |s| < 0.172 and
   ln m = 2 (s + s^3/3 + s^5/5 + s^7/7 + s^9/9), less than 7e-10 left out.
   Since 2 s = f - s f, the sum is f - s (f - p) with
   p = 2 (s^2/3 + s^4/5 + s^6/7 + s^8/9) (poly): its largest term, f, is
   exact. */
float logf(float value)
{
    uint32_t bits = to_bits(value);
    int exponent = 0;

    if (value != value || bits == INF)
        return value;                   /* not a number, +infinity */
    if (value == 0)
        return from_bits(INF | 1u << 31);
    if (value < 0)
        return from_bits(NOT_A_NUMBER);
    if (bits < 1u << 23) {              /* below 2^-126: no leading 1 is stored */
        bits = to_bits(value * 0x1p25f);
        exponent = -25;
    }
    exponent += (int)(bits >> 23) - 127;
    float mantissa = from_bits((bits & 0x007fffff) | 0x3f800000);   /* 1 <= mantissa < 2 */
    if (mantissa > 1.41421356f) {
        mantissa *= 0.5f;
        exponent++;
    }

    float delta = mantissa - 1, ratio = delta / (2 + delta), square = ratio * ratio;
    float poly = square * (2.0f / 3 + square * (2.0f / 5 + square * (2.0f / 7 +
                 square * (2.0f / 9))));
    return (float)exponent * LN2_HI +
           (delta + ((float)exponent * LN2_LO - ratio * (delta - poly)));
}

/* x^y = e^(y ln x) for x = base >= 0, y = exponent. The product y ln x is
   rounded before the exponential, so a result far from 1 is less exact than
   expf and logf are: the error grows with |y ln x|. */
float powf(float base, float exponent)
{
    if (exponent == 0)
        return 1;
    return expf(exponent * logf(base));
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

/* The angle x (angle) as n quarter turns (turns, returned) and a rest r
   (*rest): x = n pi/2 + r with |r| <= pi/4. n is x / (pi/2) rounded to an
   integer. The four parts of n pi/2 are taken off one after another: when x
   is close to n pi/2, the first three differences are exact and the rest
   keeps all its digits. This holds while n is below 512, that is for |x|
   below 800. */
static int quarter_turns(float angle, float *rest)
{
    int turns = nearest(angle * INV_PIO2);
    float multiple = (float)turns;
    *rest = (((angle - multiple * PIO2_1) - multiple * PIO2_2) - multiple * PIO2_3) -
            multiple * PIO2_4;
    return turns;
}

/* sin r for r = angle, |r| <= pi/4: r - r^3/3! + r^5/5! - r^7/7! + r^9/9!,
   less than 2e-9 left out. */
static float sin_series(float angle)
{
    float square = angle * angle;
    return angle + angle * square * (-1.0f / 6 + square * (1.0f / 120 +
           square * (-1.0f / 5040 + square * (1.0f / 362880))));
}

/* cos r for r = angle, |r| <= pi/4: 1 - r^2/2! + r^4/4! - ... - r^10/10!,
   less than 2e-10 left out. */
static float cos_series(float angle)
{
    float square = angle * angle;
    return 1 - 0.5f * square + square * square * (1.0f / 24 + square * (-1.0f / 720 +
           square * (1.0f / 40320 + square * (-1.0f / 3628800))));
}

/* The sine of n pi/2 + r for n = turns, r = rest: one of sin r, cos r,
   -sin r, -cos r. */
static float sin_turned(int turns, float rest)
{
    float value = turns & 1 ? cos_series(rest) : sin_series(rest);
    return turns & 2 ? -value : value;
}

float sinf(float angle)
{
    float rest;
    int turns = quarter_turns(angle, &rest);
    return sin_turned(turns, rest);
}

/* cos x = sin(x + pi/2): one quarter turn more. */
float cosf(float angle)
{
    float rest;
    int turns = quarter_turns(angle, &rest);
    return sin_turned(turns + 1, rest);
}
