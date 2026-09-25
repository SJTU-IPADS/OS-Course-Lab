/* nq_q4_k_fit: the scale and offset of one Q4_K sub-block. Given by the
 * framework; q4_k_quantize calls it once for each of its 8 sub-blocks.
 *
 * The search is make_qkx2_quants from ggml, called the way quantize_row_q4_K
 * calls it (nmax 15, rmin -1, rdelta 0.1, nstep 20, use_mad false):
 *
 *   1. weights  w[i] = sqrt(sum(x^2) / 32) + |x[i]|; elements far from 0 weigh more
 *   2. start    b = min(min(x), 0), hi = max(x); codes spread 0..15 over [b, hi],
 *               a = (hi - b) / 15
 *   3. search   21 candidates: code spacing (hi - b) / (14 + 0.1 k), k = 0..20.
 *               For each, round the codes q = (x - b) / spacing, then solve
 *               weighted least squares for x ~ a' * q + b', with b' forced to 0
 *               when the solution has b' > 0. When the candidate's weighted
 *               squared error is the smallest so far, (a, b) = (a', b'); the
 *               next candidates then measure from the new b, as ggml does.
 *   4. result   *scale = a, *min = -b
 *
 * Forcing b <= 0 keeps the offset non-negative, the sign the Q4_K formula
 * x = d*sc*q - dmin*m stores it with. */
#include <math.h>

#include "nq.h"

void nq_q4_k_fit(const float *x, float *scale, float *min) {
    float w[32];
    float sum_x2 = 0;
    for (int i = 0; i < 32; ++i) sum_x2 += x[i] * x[i];
    const float av_x = sqrtf(sum_x2 / 32.0f);
    for (int i = 0; i < 32; ++i) w[i] = av_x + fabsf(x[i]);

    float b = x[0], hi = x[0];
    float sum_w = w[0], sum_x = w[0] * x[0];
    for (int i = 1; i < 32; ++i) {
        if (x[i] < b)  b  = x[i];
        if (x[i] > hi) hi = x[i];
        sum_w += w[i];
        sum_x += w[i] * x[i];
    }
    if (b > 0) b = 0;
    if (hi == b) {                    /* every x[i] is the same value <= 0 */
        *scale = 0;
        *min   = -b;
        return;
    }

    /* the starting point: codes spread evenly over [b, hi] */
    float iscale = 15.0f / (hi - b);
    float a = 1.0f / iscale;
    float best = 0;
    for (int i = 0; i < 32; ++i) {
        int l = nq_round(iscale * (x[i] - b));
        l = l < 0 ? 0 : (l > 15 ? 15 : l);
        float diff = a * (float)l + b - x[i];
        best += w[i] * diff * diff;
    }

    for (int is = 0; is <= 20; ++is) {
        iscale = (-1.0f + 0.1f * (float)is + 15.0f) / (hi - b);
        uint8_t L[32];
        float sum_l = 0, sum_l2 = 0, sum_xl = 0;
        for (int i = 0; i < 32; ++i) {
            int l = nq_round(iscale * (x[i] - b));
            l = l < 0 ? 0 : (l > 15 ? 15 : l);
            L[i] = (uint8_t)l;
            sum_l  += w[i] * (float)l;
            sum_l2 += w[i] * (float)l * (float)l;
            sum_xl += w[i] * (float)l * x[i];
        }
        float D = sum_w * sum_l2 - sum_l * sum_l;
        if (!(D > 0)) continue;
        float this_a = (sum_w * sum_xl - sum_x * sum_l) / D;
        float this_b = (sum_l2 * sum_x - sum_l * sum_xl) / D;
        if (this_b > 0) {
            this_b = 0;
            this_a = sum_xl / sum_l2;
        }
        float err = 0;
        for (int i = 0; i < 32; ++i) {
            float diff = this_a * (float)L[i] + this_b - x[i];
            err += w[i] * diff * diff;
        }
        if (err < best) {
            best = err;
            a = this_a;
            b = this_b;
        }
    }
    *scale = a > 0 ? a : 0;
    *min   = -b;
}
