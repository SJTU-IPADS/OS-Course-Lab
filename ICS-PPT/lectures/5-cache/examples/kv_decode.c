/* One attention head decodes T tokens, without and with a KV cache, for several T.
   Weights are fixed pseudo-random numbers; each output is fed back as the next input.

     gcc -O2 kv_decode.c -o kv_decode -lm && ./kv_decode */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define D 128                             /* head dimension */
#define TMAX 1024                         /* the longest run */

static float WQ[D][D], WK[D][D], WV[D][D];
static float X[TMAX][D];                  /* the input vector of every position */
static float K[TMAX][D], V[TMAX][D];      /* the KV cache */
static float out_plain[TMAX][D], out_cached[TMAX][D];

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static unsigned seed = 1;
static float rnd(void) {                  /* uniform in [-0.5, 0.5) / 8 */
    seed = seed * 1664525u + 1013904223u;
    return ((seed >> 8) / 16777216.0f - 0.5f) / 8.0f;
}

static void project(const float W[D][D], const float *x, float *y) {
    for (int j = 0; j < D; j++) {
        float acc = 0;
        for (int i = 0; i < D; i++)
            acc += x[i] * W[i][j];
        y[j] = acc;
    }
}

static void compute_kv(int pos) {         /* the k and v of one token */
    project(WK, X[pos], K[pos]);
    project(WV, X[pos], V[pos]);
}

/* o = softmax(q K^T / sqrt(D)) V over positions 0..t */
static void attend(const float *q, int t, float *o) {
    float score[TMAX], max = -1e30f, sum = 0;
    for (int i = 0; i <= t; i++) {
        float acc = 0;
        for (int d = 0; d < D; d++)
            acc += q[d] * K[i][d];
        score[i] = acc / sqrtf(D);
        if (score[i] > max) max = score[i];
    }
    for (int i = 0; i <= t; i++) {
        score[i] = expf(score[i] - max);
        sum += score[i];
    }
    memset(o, 0, D * sizeof(float));
    for (int i = 0; i <= t; i++)
        for (int d = 0; d < D; d++)
            o[d] += score[i] / sum * V[i][d];
}

static void next_input(const float *o, float *x) {   /* the output becomes the next input */
    for (int d = 0; d < D; d++)
        x[d] = tanhf(o[d]);
}

/* generate T tokens; returns the time in seconds */
static double decode(int T, int cached, float (*out)[D]) {
    for (int d = 0; d < D; d++)
        X[0][d] = 0.5f;
    double start = now();
    for (int t = 0; t < T; t++) {
        float q[D];
        project(WQ, X[t], q);
        if (cached)
            compute_kv(t);                /* only the new token */
        else
            for (int i = 0; i <= t; i++)  /* every token again */
                compute_kv(i);
        attend(q, t, out[t]);
        if (t + 1 < T)
            next_input(out[t], X[t + 1]);
    }
    return now() - start;
}

int main(void) {
    static const int lengths[] = {64, 128, 256, 512, 1024};
    int identical = 1;
    for (int i = 0; i < D; i++)
        for (int j = 0; j < D; j++) {
            WQ[i][j] = rnd();
            WK[i][j] = rnd();
            WV[i][j] = rnd();
        }
    printf("tokens   no cache   KV cache   speedup\n");
    for (int n = 0; n < (int)(sizeof lengths / sizeof lengths[0]); n++) {
        int T = lengths[n];
        double t_plain = decode(T, 0, out_plain);
        double t_cached = decode(T, 1, out_cached);
        identical &= memcmp(out_plain, out_cached, T * sizeof out_plain[0]) == 0;
        printf("%6d %8.1f ms %8.1f ms %8.1f\n",
               T, t_plain * 1e3, t_cached * 1e3, t_plain / t_cached);
    }
    printf("outputs identical: %s\n", identical ? "yes" : "no");
    return 0;
}
