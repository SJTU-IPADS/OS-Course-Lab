/* ops-selftest -- checks a set of operators against ops/scalar.c and the
   definitions in ops.h, and times its matvec against the scalar one; needs
   no model file.

     ./ops-selftest-simd       the operators of ops/simd.c
     ./ops-selftest-cuda       the operators of ops/cuda.cu

   The set is chosen when the program is built, as for mini-ollama: the
   Makefile compiles this file with -DOPS=ops_simd or -DOPS=ops_cuda.

   1. operators   embed, rmsnorm, rope, the attention, swiglu, add and
                  argmax, on fixed-seed pseudo-random inputs of the sizes
                  the model uses and of other sizes (not multiples of 8,
                  shorter than a block of threads), against the same
                  operator of ops/scalar.c.
   2. matvec      the operator you write: matvec_q4_0 in ops/simd.c, or
                  matvec_q4_0_kernel, which matvec_q4_0 in ops/cuda.cu
                  launches. Q4_0 matrices of the widths the model uses and
                  narrower ones, row by row against the sum computed in
                  double from the definition in ops.h.
   3. speed       matvec on the matrices of one layer and on the output
                  matrix (151936 x 2048, 175 MB), with the set and with the
                  scalar operator on one CPU thread: milliseconds per
                  product, and the bytes of weights read per second. The
                  speed-up on attn_q has to reach required_speedup().

   One line per item, "pass" or "FAIL"; the exit status is the number of
   failed items. An operator that is not written yet stops the program at
   its first call (ops/simd.c) or after its first launch (ops/cuda.cu): the
   operators of group 1 are all written in both sets.

   The inputs are made in the memory of the program and copied into the
   memory of the set with upload(); the results are copied back with
   download(). An input that the scalar operator changes in place is copied
   before that call. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mini_ollama.h"
#include "ops.h"

#ifndef OPS
#error "OPS names the set of operators: build with make, which gives -DOPS=ops_simd and so on"
#endif

/* The error allowed, as a fraction: of the largest |value| of the output,
   and for matvec of the sum of |W[r][c] * x[c]| over a row. Room for a
   different order of float additions and for an exponential or a sine
   within a few units in the last place; far below the error of any wrong
   decoding (one weight left out of 2048 is 1/2048). */
#define TOLERANCE 1e-5

static const struct ops *S = &ops_scalar, *T = &OPS;     /* T: the set tested */

/* Fixed-seed pseudo-random numbers: the same sequence on every machine. */
static uint64_t seed = 20261005;

static uint32_t next(void)
{
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return (uint32_t)(seed >> 32);
}

static float uniform(void)              /* [0, 1) */
{
    return (float)(next() >> 8) / (float)(1 << 24);
}

/* A random Q4_0 row of n weights: scales of both signs between 2^-10 and
   2^5, every code value equally likely. */
static void random_row(uint8_t *blocks, size_t n)
{
    for (size_t b = 0; b < n / 32; b++, blocks += 18) {
        uint32_t h = (next() & 1) << 15 | (5 + next() % 16) << 10 | (next() & 1023);
        blocks[0] = (uint8_t)h;
        blocks[1] = (uint8_t)(h >> 8);
        for (int j = 0; j < 16; j++)
            blocks[2 + j] = (uint8_t)(next() >> 24);
    }
}

static void random_vector(float *x, size_t n, float range)
{
    for (size_t i = 0; i < n; i++)
        x[i] = (uniform() * 2 - 1) * range;
}

static void *alloc(size_t bytes)
{
    void *p = malloc(bytes ? bytes : 1);
    if (!p)
        die("out of memory");
    return p;
}

/* Memory of the tested set holding a copy of the bytes at host. A CPU set
   returns the memory it is given, so the bytes are copied first: the
   scalar operator may change host afterwards. */
static void *up(const void *host, size_t bytes)
{
    void *copy = alloc(bytes), *mem;
    memcpy(copy, host, bytes);
    mem = T->upload(copy, bytes);
    if (mem != copy)
        free(copy);
    return mem;
}

/* n floats of memory of the tested set; the n floats at mem back in host. */
static float *room(size_t n)
{
    return T->alloc(n * sizeof(float));
}

static void down(float *host, const float *mem, size_t n)
{
    T->download(host, mem, n * sizeof(float));
}

static int fails;

static void verdict(int ok, const char *what)
{
    printf("  %s  %s\n", ok ? "pass" : "FAIL", what);
    if (!ok)
        fails++;
}

/* ---------------- 1. operators ---------------- */

/* max |got - want| over max |want|, the largest over the calls of one
   item. A NaN counts as infinite. */
static double worst;

static void compare(const float *got, const float *want, size_t n)
{
    double big = 0, err = 0;
    for (size_t i = 0; i < n; i++) {
        double d = fabs((double)got[i] - want[i]);
        if (fabs(want[i]) > big)
            big = fabs(want[i]);
        if (!(d <= err))
            err = d != d ? INFINITY : d;
    }
    err /= big + 1e-30;
    if (!(err <= worst))
        worst = err != err ? INFINITY : err;
}

static void report_item(const char *name, const char *sizes)
{
    char what[160];
    snprintf(what, sizeof what, "%s: %s, error %.1e of the largest |value|", name, sizes, worst);
    verdict(worst <= TOLERANCE, what);
    worst = 0;
}

static void check_embed(void)
{
    static const size_t ns[] = {32, 2048};
    for (int c = 0; c < 2; c++) {
        size_t n = ns[c];
        uint8_t *row = alloc(n / 32 * 18);
        float *a = alloc(n * sizeof *a), *b = alloc(n * sizeof *b), *tb = room(n);
        random_row(row, n);
        S->embed(a, row, n);
        T->embed(tb, up(row, n / 32 * 18), n);
        down(b, tb, n);
        compare(b, a, n);
        free(row); free(a); free(b);
    }
    report_item("embed", "32 and 2048 weights");
}

static void check_rope(void)
{
    static const int shape[][3] = {{16, 128, 0}, {8, 128, 37}, {16, 128, 511}, {3, 20, 5}};
    for (int c = 0; c < 4; c++) {
        int n_vec = shape[c][0], d = shape[c][1], pos = shape[c][2];
        size_t n = (size_t)n_vec * d;
        float *a = alloc(n * sizeof *a), *b = alloc(n * sizeof *b), *tb;
        random_vector(a, n, 4);
        tb = up(a, n * sizeof *a);
        S->rope(a, n_vec, d, pos, 5e6f);
        T->rope(tb, n_vec, d, pos, 5e6f);
        down(b, tb, n);
        compare(b, a, n);
        free(a); free(b);
    }
    report_item("rope", "16 x 128 at positions 0 and 511, 8 x 128 at 37, 3 x 20");
}

/* The attention of n_head query heads over n_tok tokens: scores, softmax,
   values, each against the scalar operator on the same input. */
static void attention_case(int n_head, int n_head_kv, int d, int n_tok,
                           double *e_scores, double *e_softmax, double *e_values)
{
    size_t nq = (size_t)n_head * d, nc = (size_t)n_tok * n_head_kv * d, ns = (size_t)n_head * n_tok;
    float *q = alloc(nq * sizeof *q), *k = alloc(nc * sizeof *k), *v = alloc(nc * sizeof *v);
    float *sa = alloc(ns * sizeof *sa), *sb = alloc(ns * sizeof *sb);
    float *oa = alloc(nq * sizeof *oa), *ob = alloc(nq * sizeof *ob);
    float *ts = room(ns), *to = room(nq);
    random_vector(q, nq, 3);
    random_vector(k, nc, 3);
    random_vector(v, nc, 2);

    S->attn_scores(sa, q, k, n_head, n_head_kv, d, n_tok);
    T->attn_scores(ts, up(q, nq * sizeof *q), up(k, nc * sizeof *k), n_head, n_head_kv, d, n_tok);
    down(sb, ts, ns);
    worst = 0; compare(sb, sa, ns); if (worst > *e_scores) *e_scores = worst;

    ts = up(sa, ns * sizeof *sa);       /* the same scores for both */
    S->softmax(sa, n_head, n_tok);
    T->softmax(ts, n_head, n_tok);
    down(sb, ts, ns);
    worst = 0; compare(sb, sa, ns); if (worst > *e_softmax) *e_softmax = worst;

    S->attn_values(oa, sa, v, n_head, n_head_kv, d, n_tok);
    T->attn_values(to, up(sa, ns * sizeof *sa), up(v, nc * sizeof *v), n_head, n_head_kv, d, n_tok);
    down(ob, to, nq);
    worst = 0; compare(ob, oa, nq); if (worst > *e_values) *e_values = worst;

    free(q); free(k); free(v); free(sa); free(sb); free(oa); free(ob);
}

static void check_attention(void)
{
    static const int shape[][4] = {{16, 8, 128, 1}, {16, 8, 128, 37}, {16, 8, 128, 512}, {4, 2, 36, 13}};
    double es = 0, ex = 0, ev = 0;
    for (int c = 0; c < 4; c++)
        attention_case(shape[c][0], shape[c][1], shape[c][2], shape[c][3], &es, &ex, &ev);
    const char *sizes = "16 heads (8 key heads) x 128 over 1, 37, 512 tokens; 4 (2) x 36 over 13";
    worst = es; report_item("attn_scores", sizes);
    worst = ex; report_item("softmax", sizes);
    worst = ev; report_item("attn_values", sizes);

    /* softmax of a row whose scores spread over 200: most weights are 0 */
    float a[37], b[37], *tb;
    random_vector(a, 37, 100);
    tb = up(a, sizeof a);
    S->softmax(a, 1, 37);
    T->softmax(tb, 1, 37);
    down(b, tb, 37);
    compare(b, a, 37);
    report_item("softmax", "37 scores between -100 and 100");
}

static void check_swiglu(void)
{
    static const int ns[] = {6144, 13};
    for (int c = 0; c < 2; c++) {
        int n = ns[c];
        float *g = alloc(n * sizeof *g), *u = alloc(n * sizeof *u), *b = alloc(n * sizeof *b), *tg;
        random_vector(g, n, 20);
        random_vector(u, n, 4);
        g[0] = -100;                    /* e^100 is infinite in float */
        tg = up(g, n * sizeof *g);
        S->swiglu(g, u, n);
        T->swiglu(tg, up(u, n * sizeof *u), n);
        down(b, tg, n);
        compare(b, g, n);
        free(g); free(u); free(b);
    }
    report_item("swiglu", "6144 and 13 values between -20 and 20, one at -100");
}

static void check_add(void)
{
    static const int ns[] = {2048, 13};
    for (int c = 0; c < 2; c++) {
        int n = ns[c];
        float *x = alloc(n * sizeof *x), *y = alloc(n * sizeof *y), *b = alloc(n * sizeof *b), *tx;
        random_vector(x, n, 4);
        random_vector(y, n, 4);
        tx = up(x, n * sizeof *x);
        S->add(x, y, n);
        T->add(tx, up(y, n * sizeof *y), n);
        down(b, tx, n);
        compare(b, x, n);
        free(x); free(y); free(b);
    }
    report_item("add", "2048 and 13 values");
}

static void check_argmax(void)
{
    enum { N = 151936 };                /* n_vocab: the logits */
    float *x = alloc(N * sizeof *x), *tx;
    int ok = 1;
    random_vector(x, N, 10);
    ok &= S->argmax(x, N) == T->argmax(up(x, N * sizeof *x), N);
    x[1000] = x[90000] = 11;            /* two equal largest: the first counts */
    ok &= T->argmax(up(x, N * sizeof *x), N) == 1000;
    x[N - 1] = 12;                      /* the last of 13 */
    tx = up(x, N * sizeof *x);
    ok &= T->argmax(tx + N - 13, 13) == 12;
    x[0] = 13;
    ok &= T->argmax(up(x, N * sizeof *x), N) == 0;
    verdict(ok, "argmax: 151936 values, two equal largest, the largest first and last");
    free(x);
}

static void check_rmsnorm(void)
{
    static const int shape[][2] = {{1, 2048}, {16, 128}, {8, 128}, {3, 100}, {2, 1000}};
    for (int c = 0; c < 5; c++) {
        int n_vec = shape[c][0], d = shape[c][1];
        size_t n = (size_t)n_vec * d;
        float *x = alloc(n * sizeof *x), *g = alloc(d * sizeof *g);
        float *a = alloc(n * sizeof *a), *b = alloc(n * sizeof *b);
        float *tx, *tg, *ty = room(n);
        random_vector(x, n, 4);
        random_vector(g, d, 2);
        tx = up(x, n * sizeof *x);
        tg = up(g, d * sizeof *g);
        S->rmsnorm(a, x, g, n_vec, d, 1e-6f);
        T->rmsnorm(ty, tx, tg, n_vec, d, 1e-6f);
        down(b, ty, n);
        compare(b, a, n);
        T->rmsnorm(tx, tx, tg, n_vec, d, 1e-6f);    /* in place: y = x */
        down(b, tx, n);
        compare(b, a, n);
        free(x); free(g); free(a); free(b);
    }
    report_item("rmsnorm", "1 x 2048, 16 x 128, 8 x 128, 3 x 100, 2 x 1000, also in place");
}

/* ---------------- 2. matvec ---------------- */

/* The sum from the definition, in double, and the sum of |terms| (for the
   tolerance). */
static double exact(const uint8_t *blocks, const float *x, size_t n, double *sum_abs)
{
    double sum = 0, abs = 0;
    for (size_t b = 0; b < n / 32; b++, blocks += 18) {
        double d = q4_0_scale(blocks);
        for (int j = 0; j < 16; j++) {
            double lo = d * ((blocks[2 + j] & 15) - 8) * x[32 * b + j];
            double hi = d * ((blocks[2 + j] >> 4) - 8) * x[32 * b + 16 + j];
            sum += lo + hi;
            abs += fabs(lo) + fabs(hi);
        }
    }
    *sum_abs = abs;
    return sum;
}

/* matvec of a rows x cols matrix, row by row against the definition. Row 0
   holds every code in both halves of a byte with d = -1; row 1 has d = 0
   in its first block and the smallest subnormal d in its second. */
static void check_matvec(size_t rows, size_t cols, float range)
{
    size_t row_bytes = cols / 32 * 18;
    uint8_t *w = alloc(rows * row_bytes);
    float *x = alloc(cols * sizeof *x), *y = alloc(rows * sizeof *y), *ty = room(rows);
    double err = 0;
    char what[160];
    for (size_t r = 0; r < rows; r++)
        random_row(w + r * row_bytes, cols);
    random_vector(x, cols, range);
    for (int j = 0; j < 16; j++)
        w[2 + j] = (uint8_t)(j | (15 - j) << 4);
    w[0] = 0x00; w[1] = 0xbc;                   /* d = -1 */
    if (rows > 1) {
        uint8_t *row1 = w + row_bytes;
        row1[0] = row1[1] = 0;                  /* d = 0 */
        if (cols >= 64) {
            row1[18] = 0x01; row1[19] = 0x00;   /* the smallest subnormal */
        }
    }
    T->matvec(ty, up(w, rows * row_bytes), up(x, cols * sizeof *x), rows, cols);
    down(y, ty, rows);
    for (size_t r = 0; r < rows; r++) {
        double sum_abs, want = exact(w + r * row_bytes, x, cols, &sum_abs);
        double e = fabs(y[r] - want) / (sum_abs + 1e-30);
        if (!(e <= err))
            err = e != e ? INFINITY : e;
    }
    snprintf(what, sizeof what, "matvec, %zu x %zu%s: error %.1e of the sum of |terms|",
             rows, cols, range > 1 ? ", x up to 64" : "", err);
    verdict(err <= TOLERANCE, what);
    free(w); free(x); free(y);
}

/* ---------------- 3. speed ---------------- */

/* The speed-up of matvec over the scalar operator on attn_q (2048 x 2048,
   2.4 MB, inside the cache) that the tested set has to reach; 0 for none.
   ops/simd.c: 4. An implementation that still takes one weight at a time
   reaches 1.6 with -mavx2, one that takes the 32 weights of a block per
   step 6 to 7. ops/cuda.cu: none; the speed of a GPU depends on the
   card. */
static double required_speedup(void)
{
    return strncmp(T->name, "simd", 4) == 0 ? 4.0 : 0;
}

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* Seconds per matrix-vector product, the best of 3 trials of at least
   0.2 s each. A kernel launch returns before the kernel has run: finish()
   waits for every launch of the trial. */
static double time_matvec(const struct ops *o, float *y, const uint8_t *w, const float *x,
                          size_t rows, size_t cols)
{
    double best = INFINITY;
    o->matvec(y, w, x, rows, cols);     /* the first launch loads the kernel */
    o->finish();
    for (int t = 0; t < 3; t++) {
        int reps = 0;
        double t0 = now(), dt;
        do {
            o->matvec(y, w, x, rows, cols);
            reps++;
            if (reps % 16 == 0 || o == S)
                o->finish();
        } while ((dt = now() - t0) < 0.2);
        o->finish();
        dt = now() - t0;
        if (dt / reps < best)
            best = dt / reps;
    }
    return best;
}

static void check_speed(int set_len)
{
    static const struct { size_t rows, cols; const char *what; } m[] = {
        {2048, 2048, "attn_q"},
        {6144, 2048, "ffn_gate, ffn_up"},
        {2048, 6144, "ffn_down"},
        {151936, 2048, "output"},
    };
    double speedup = 0, need = required_speedup();
    char head[32], what[128];
    snprintf(head, sizeof head, "%.*s ms  GB/s", set_len, T->name);
    printf("  %-16s %-14s %8s %16s %16s %9s\n", "matrix", "rows x cols", "MB", "scalar ms  GB/s", head, "speed-up");
    for (int i = 0; i < 4; i++) {
        size_t rows = m[i].rows, cols = m[i].cols, bytes = rows * (cols / 32 * 18);
        uint8_t *w = alloc(bytes);
        float *x = alloc(cols * sizeof *x), *y = alloc(rows * sizeof *y);
        char shape[32];
        for (size_t r = 0; r < rows; r++)
            random_row(w + r * (cols / 32 * 18), cols);
        random_vector(x, cols, 1);
        double ts = time_matvec(S, y, w, x, rows, cols);
        double tt = time_matvec(T, room(rows), up(w, bytes), up(x, cols * sizeof *x), rows, cols);
        snprintf(shape, sizeof shape, "%zu x %zu", rows, cols);
        printf("  %-16s %-14s %8.1f %8.3f %7.1f %8.3f %7.1f %8.1fx\n", m[i].what, shape, bytes / 1e6,
               ts * 1e3, bytes / ts / 1e9, tt * 1e3, bytes / tt / 1e9, ts / tt);
        if (i == 0)
            speedup = ts / tt;
        free(w); free(x); free(y);
    }
    if (need > 0) {
        snprintf(what, sizeof what, "speed-up on attn_q %.1fx, at least %.0fx required", speedup, need);
        verdict(speedup >= need, what);
    }
}

int main(void)
{
    int set_len = (int)strcspn(T->name, " ");       /* "simd", "cuda" */

    setvbuf(stdout, NULL, _IONBF, 0);   /* keep the order with the messages on stderr */
    printf("operators: %s\n", T->name);
    printf("device: %s\n", T->init());
    S->init();

    printf("1. operators against ops/scalar.c\n");
    check_embed();
    check_rmsnorm();
    check_rope();
    check_attention();
    check_swiglu();
    check_add();
    check_argmax();

    printf("2. matvec against the definition\n");
    check_matvec(64, 32, 1);
    check_matvec(64, 96, 1);
    check_matvec(1000, 2048, 1);        /* attn_q, ffn_gate: n_embd columns */
    check_matvec(1000, 2048, 64);       /* activations of a larger magnitude */
    check_matvec(64, 6144, 1);          /* ffn_down: n_ff columns */

    printf("3. speed of matvec: %.*s against the scalar operator on one CPU thread\n", set_len, T->name);
    check_speed(set_len);

    printf("\n");
    if (fails == 0)
        printf("all passed\n");
    else
        printf("%d failed\n", fails);
    return fails;
}
