/* One attention head decodes T tokens, without and with a KV cache.
   Weights are fixed pseudo-random numbers; each output is fed back as the next input.

     gcc -O2 kv_decode.c -o kv_decode -lm && ./kv_decode */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define D 64                              /* head dimension */
#define T 1024                            /* tokens to generate */

static float WQ[D][D], WK[D][D], WV[D][D];
static float X[T][D];                     /* the input vector of every position */
static float K[T][D], V[T][D];            /* the KV cache */
static float out_plain[T][D], out_cached[T][D];
static long projections;                  /* k, v computed so far */

static double now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static unsigned seed = 1;
static float rnd(void) {                  /* uniform in [-0.5, 0.5) / sqrt(D) */
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
    projections++;
}

/* o = softmax(q K^T / sqrt(D)) V over positions 0..t */
static void attend(const float *q, int t, float *o) {
    float score[T], max = -1e30f, sum = 0;
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

static double decode(int cached, float (*out)[D]) {
    projections = 0;
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
    for (int i = 0; i < D; i++)
        for (int j = 0; j < D; j++) {
            WQ[i][j] = rnd();
            WK[i][j] = rnd();
            WV[i][j] = rnd();
        }
    double t_plain = decode(0, out_plain);
    long n_plain = projections;
    double t_cached = decode(1, out_cached);
    long n_cached = projections;
    printf("tokens %d, head dimension %d\n", T, D);
    printf("no cache  %8ld k,v projections  %8.1f ms\n", n_plain, t_plain * 1e3);
    printf("KV cache  %8ld k,v projections  %8.1f ms\n", n_cached, t_cached * 1e3);
    printf("outputs identical: %s\n",
           memcmp(out_plain, out_cached, sizeof out_plain) == 0 ? "yes" : "no");
    return 0;
}
