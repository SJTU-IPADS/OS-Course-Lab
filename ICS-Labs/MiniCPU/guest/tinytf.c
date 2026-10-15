/* A tiny transformer in integer arithmetic: one causal self-attention layer
 * and one MLP, generating tokens greedily. Every matrix-vector product is
 * linear_simd of guest/linear.s, so the program runs on the vector subset
 * of lab 1.
 *
 *   tinytf [seed [n]]      seed selects the weights and the prompt (default 1),
 *                          n is the number of generated tokens, 1 .. 12 (default 8)
 *
 * Output:
 *   prompt  = four tokens
 *   tokens  = the n generated tokens
 *   logits  = the sum of all logits computed, a check on every step
 *
 * Sizes: vocabulary 16, width 16 (two vector iterations per product),
 * hidden 36 (four iterations and a tail of four). Weights are integers in
 * [-8, 7] from xorshift32; products are scaled back by an arithmetic shift
 * of 4 and activations are clamped to [-1024, 1024], so no int overflows
 * (the largest intermediate value, q.k, stays below 2^30).
 * tests/reference.py tinytf computes the same output.
 */
#include "mini.h"

#define V 16            /* vocabulary */
#define D 16            /* width */
#define H 36            /* hidden width of the MLP */
#define P 4             /* prompt length */
#define T (P + 12)      /* longest sequence */
#define LIM 1024

int linear_simd(const int *w, const int *x, int b, int n);

static int emb[V][D], pos[T][D];
static int wq[D][D], wk[D][D], wv[D][D], wo[D][D];
static int w1[H][D], b1[H], w2[D][H], b2[D];
static int un[V][D];
static int keys[T][D], vals[T][D];

static unsigned rng;

static int weight(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (int)(rng % 16) - 8;
}

static void fill(int *m, int n)
{
    for (int i = 0; i < n; ++i)
        m[i] = weight();
}

static int clamp(int v)
{
    return v < -LIM ? -LIM : v > LIM ? LIM : v;
}

/* y = (W x + b) >> 4 for an R x C matrix W */
static void mul(int *y, const int *w, const int *b, const int *x, int r, int c)
{
    for (int i = 0; i < r; ++i)
        y[i] = linear_simd(w + i * c, x, b ? b[i] : 0, c) >> 4;
}

static unsigned number(const char *s)
{
    unsigned v = 0;
    for (; *s >= '0' && *s <= '9'; ++s)
        v = v * 10 + (unsigned)(*s - '0');
    return v;
}

static void putnums(const char *name, const int *a, int n)
{
    char buf[3];
    puts(name);
    for (int i = 0; i < n; ++i) {       /* tokens are 0 .. 15 */
        int k = 0;
        buf[k++] = ' ';
        if (a[i] >= 10)
            buf[k++] = (char)('0' + a[i] / 10);
        buf[k++] = (char)('0' + a[i] % 10);
        write(1, buf, (size_t)k);
    }
    puts("\n");
}

int guest_main(int argc, char **argv)
{
    unsigned seed = argc > 1 ? number(argv[1]) : 1;
    int n = argc > 2 ? (int)number(argv[2]) : 8;
    if (n < 1 || n > T - P) n = 8;
    rng = seed * 2654435761u ^ 0x9e3779b9u;
    if (rng == 0) rng = 1;

    fill(emb[0], V * D);
    fill(pos[0], T * D);
    fill(wq[0], D * D);
    fill(wk[0], D * D);
    fill(wv[0], D * D);
    fill(wo[0], D * D);
    fill(w1[0], H * D);
    fill(b1, H);
    fill(w2[0], D * H);
    fill(b2, D);
    fill(un[0], V * D);

    int tok[T];
    for (int i = 0; i < P; ++i)
        tok[i] = weight() + 8;

    long total = 0;
    int x[D], q[D], a[D], o[D], h[H], y[D], logit[V];
    for (int t = 0; t < P + n - 1; ++t) {
        for (int j = 0; j < D; ++j)
            x[j] = emb[tok[t]][j] + pos[t][j];

        /* attention over positions 0 .. t: the weight of s is
           min(max(q.k >> 16, 0), 15) + 1, the average is truncated */
        mul(q, wq[0], 0, x, D, D);
        mul(keys[t], wk[0], 0, x, D, D);
        mul(vals[t], wv[0], 0, x, D, D);
        int sum = 0;
        for (int j = 0; j < D; ++j)
            a[j] = 0;
        for (int s = 0; s <= t; ++s) {
            int e = linear_simd(q, keys[s], 0, D) >> 16;
            e = (e < 0 ? 0 : e > 15 ? 15 : e) + 1;
            sum += e;
            for (int j = 0; j < D; ++j)
                a[j] += e * vals[s][j];
        }
        for (int j = 0; j < D; ++j)
            a[j] /= sum;
        mul(o, wo[0], 0, a, D, D);
        for (int j = 0; j < D; ++j)
            x[j] = clamp(x[j] + o[j]);

        /* MLP */
        mul(h, w1[0], b1, x, H, D);
        for (int j = 0; j < H; ++j)
            if (h[j] < 0) h[j] = 0;
        mul(y, w2[0], b2, h, D, H);
        for (int j = 0; j < D; ++j)
            x[j] = clamp(x[j] + y[j]);

        if (t < P - 1)
            continue;
        int best = 0;
        for (int v = 0; v < V; ++v) {
            logit[v] = linear_simd(un[v], x, 0, D);
            total += logit[v];
            if (logit[v] > logit[best]) best = v;
        }
        tok[t + 1] = best;
    }

    putnums("prompt =", tok, P);
    putnums("tokens =", tok + P, n);
    puts("logits = ");
    putint(total);
    return 0;
}
