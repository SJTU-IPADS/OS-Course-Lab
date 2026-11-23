/* A tiny transformer in integer arithmetic: one causal self-attention layer
 * and one MLP, generating tokens greedily. Every matrix-vector product is
 * linear_simd of guest/linear.s, so the program runs on the vector subset
 * of lab 1.
 *
 *   tinytf [seed [count]]  seed selects the weights and the prompt (default 1),
 *                          count is the number of generated tokens, 1 .. 12
 *                          (default 8)
 *
 * Output:
 *   prompt  = four tokens
 *   tokens  = the count generated tokens
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

#define VOCAB 16                /* vocabulary */
#define DIM 16                  /* width */
#define HIDDEN 36               /* hidden width of the MLP */
#define PROMPT 4                /* prompt length */
#define LONGEST (PROMPT + 12)   /* longest sequence */
#define LIM 1024

int linear_simd(const int *weights, const int *input, int bias, int count);

static int emb[VOCAB][DIM], pos[LONGEST][DIM];
static int wq[DIM][DIM], wk[DIM][DIM], wv[DIM][DIM], wo[DIM][DIM];
static int w1[HIDDEN][DIM], b1[HIDDEN], w2[DIM][HIDDEN], b2[DIM];
static int un[VOCAB][DIM];
static int keys[LONGEST][DIM], vals[LONGEST][DIM];

static unsigned rng;

static int weight(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return (int)(rng % 16) - 8;
}

static void fill(int *values, int count)
{
    for (int idx = 0; idx < count; ++idx)
        values[idx] = weight();
}

static int clamp(int value)
{
    return value < -LIM ? -LIM : value > LIM ? LIM : value;
}

/* out = (weights * vector + bias) >> 4; weights is nrows x ncols */
static void mul(int *out, const int *weights, const int *bias,
                const int *vector, int nrows, int ncols)
{
    for (int row = 0; row < nrows; ++row)
        out[row] = linear_simd(weights + row * ncols, vector,
                               bias ? bias[row] : 0, ncols) >> 4;
}

static unsigned number(const char *str)
{
    unsigned value = 0;
    for (; *str >= '0' && *str <= '9'; ++str)
        value = value * 10 + (unsigned)(*str - '0');
    return value;
}

static void putnums(const char *name, const int *values, int count)
{
    char buf[3];
    puts(name);
    for (int idx = 0; idx < count; ++idx) {     /* tokens are 0 .. 15 */
        int len = 0;
        buf[len++] = ' ';
        if (values[idx] >= 10)
            buf[len++] = (char)('0' + values[idx] / 10);
        buf[len++] = (char)('0' + values[idx] % 10);
        write(1, buf, (size_t)len);
    }
    puts("\n");
}

int guest_main(int argc, char **argv)
{
    unsigned seed = argc > 1 ? number(argv[1]) : 1;
    int count = argc > 2 ? (int)number(argv[2]) : 8;
    if (count < 1 || count > LONGEST - PROMPT) count = 8;
    rng = seed * 2654435761u ^ 0x9e3779b9u;
    if (rng == 0) rng = 1;

    fill(emb[0], VOCAB * DIM);
    fill(pos[0], LONGEST * DIM);
    fill(wq[0], DIM * DIM);
    fill(wk[0], DIM * DIM);
    fill(wv[0], DIM * DIM);
    fill(wo[0], DIM * DIM);
    fill(w1[0], HIDDEN * DIM);
    fill(b1, HIDDEN);
    fill(w2[0], DIM * HIDDEN);
    fill(b2, DIM);
    fill(un[0], VOCAB * DIM);

    int tok[LONGEST];
    for (int idx = 0; idx < PROMPT; ++idx)
        tok[idx] = weight() + 8;

    long total = 0;
    int residual[DIM], query[DIM], attended[DIM], attn_out[DIM];
    int hidden[HIDDEN], mlp_out[DIM], logit[VOCAB];
    for (int step = 0; step < PROMPT + count - 1; ++step) {
        for (int col = 0; col < DIM; ++col)
            residual[col] = emb[tok[step]][col] + pos[step][col];

        /* attention over the positions past = 0 .. step, each with the
           weight min(max(q.k >> 16, 0), 15) + 1; the average is truncated */
        mul(query, wq[0], 0, residual, DIM, DIM);
        mul(keys[step], wk[0], 0, residual, DIM, DIM);
        mul(vals[step], wv[0], 0, residual, DIM, DIM);
        int sum = 0;
        for (int col = 0; col < DIM; ++col)
            attended[col] = 0;
        for (int past = 0; past <= step; ++past) {
            int score = linear_simd(query, keys[past], 0, DIM) >> 16;
            score = (score < 0 ? 0 : score > 15 ? 15 : score) + 1;
            sum += score;
            for (int col = 0; col < DIM; ++col)
                attended[col] += score * vals[past][col];
        }
        for (int col = 0; col < DIM; ++col)
            attended[col] /= sum;
        mul(attn_out, wo[0], 0, attended, DIM, DIM);
        for (int col = 0; col < DIM; ++col)
            residual[col] = clamp(residual[col] + attn_out[col]);

        /* MLP */
        mul(hidden, w1[0], b1, residual, HIDDEN, DIM);
        for (int col = 0; col < HIDDEN; ++col)
            if (hidden[col] < 0) hidden[col] = 0;
        mul(mlp_out, w2[0], b2, hidden, DIM, HIDDEN);
        for (int col = 0; col < DIM; ++col)
            residual[col] = clamp(residual[col] + mlp_out[col]);

        if (step < PROMPT - 1)
            continue;
        int best = 0;
        for (int token = 0; token < VOCAB; ++token) {
            logit[token] = linear_simd(un[token], residual, 0, DIM);
            total += logit[token];
            if (logit[token] > logit[best]) best = token;
        }
        tok[step + 1] = best;
    }

    putnums("prompt =", tok, PROMPT);
    putnums("tokens =", tok + PROMPT, count);
    puts("logits = ");
    putint(total);
    return 0;
}
