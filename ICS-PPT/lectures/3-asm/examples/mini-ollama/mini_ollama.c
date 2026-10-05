/* mini-ollama: generate text from a quantized model, one weight at a time.

     make
     ./mini-ollama MODEL.gguf PROMPT [MAX_NEW_TOKENS]

   MODEL.gguf is the file the nano-quant lab builds with --recipe q4_0 and
   ollama loads: the language model of Qwen3-VL-2B-Instruct, every matrix in
   Q4_0. The answer goes to standard output as it is generated, the timing to
   standard error, under the names `ollama run --verbose` prints.

   Every step is written in the plainest way: one thread, and matvec()
   decodes one weight, multiplies it and adds it. A generated token takes 197
   products of a matrix with a vector (7 in each of the 28 layers and one for
   the output), so nearly all the time is spent in matvec(). The next token
   is the one with the largest score, which is what ollama does at
   temperature 0. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mini_ollama.h"

static uint64_t n_weights;              /* weights multiplied so far */

/* y = W x, one weight at a time. A Q4_0 block holds 32 weights in 18 bytes:
   a half-precision scale d, then 16 bytes. Byte j has weight j in its low
   4 bits and weight j + 16 in its high 4 bits; a stored value q means the
   weight d * (q - 8). */
static void matvec(float *y, const struct tensor *w, const float *x)
{
    const uint8_t *blk = w->data;
    for (size_t r = 0; r < w->rows; r++) {
        float sum = 0;
        for (size_t c = 0; c < w->cols; c += 32, blk += 18) {
            float d = fp16[blk[0] | blk[1] << 8];
            for (int j = 0; j < 16; j++) {
                int lo = (blk[2 + j] & 15) - 8, hi = (blk[2 + j] >> 4) - 8;
                sum += d * lo * x[c + j] + d * hi * x[c + j + 16];
            }
        }
        y[r] = sum;
    }
    n_weights += w->rows * w->cols;
}

/* x = row tok of the embedding matrix. */
static void embed(const struct model *m, int tok, float *x)
{
    const struct tensor *w = &m->token_embd;
    const uint8_t *blk = w->data + (size_t)tok * (w->cols / 32 * 18);
    for (size_t c = 0; c < w->cols; c += 32, blk += 18) {
        float d = fp16[blk[0] | blk[1] << 8];
        for (int j = 0; j < 16; j++) {
            x[c + j]      = d * ((blk[2 + j] & 15) - 8);
            x[c + j + 16] = d * ((blk[2 + j] >> 4) - 8);
        }
    }
}

/* y = x scaled to root mean square 1, times the gain g. y may be x. */
static void rmsnorm(float *y, const float *x, const struct tensor *g, float eps)
{
    const float *gain = (const float *)g->data;
    float ss = 0;
    for (size_t i = 0; i < g->cols; i++)
        ss += x[i] * x[i];
    float scale = 1 / sqrtf(ss / g->cols + eps);
    for (size_t i = 0; i < g->cols; i++)
        y[i] = x[i] * scale * gain[i];
}

/* Mark v with its position: the pair (v[i], v[i + d/2]) turns by the angle
   pos * base^(-2i/d). */
static void rope(float *v, int d, int pos, float base)
{
    for (int i = 0; i < d / 2; i++) {
        float angle = pos * powf(base, -2.0f * i / d);
        float c = cosf(angle), s = sinf(angle);
        float a = v[i], b = v[i + d / 2];
        v[i]         = a * c - b * s;
        v[i + d / 2] = b * c + a * s;
    }
}

static float dot(const float *a, const float *b, int n)
{
    float sum = 0;
    for (int i = 0; i < n; i++)
        sum += a[i] * b[i];
    return sum;
}

/* Turn n scores into n weights that are positive and add up to 1. */
static void softmax(float *s, int n)
{
    float max = s[0], sum = 0;
    for (int i = 1; i < n; i++)
        if (s[i] > max)
            max = s[i];
    for (int i = 0; i < n; i++)
        sum += s[i] = expf(s[i] - max);
    for (int i = 0; i < n; i++)
        s[i] /= sum;
}

/* x += what the token at pos takes from the tokens up to pos.

   The token asks with a query q; every token so far answers with its key k
   and value v, kept in the cache of this layer. A head adds up the values,
   each weighted by how well its key matches the query. There are n_head
   query heads and n_head_kv key heads: n_head / n_head_kv query heads share
   one key head. */
static void attention(struct model *m, int l, int pos, float *x)
{
    struct layer *L = &m->layer[l];
    int d = m->head_dim, kv = m->n_head_kv * d;
    float h[m->n_embd], q[m->n_head * d], sum[m->n_head * d], y[m->n_embd];
    float *k = L->k_cache + (size_t)pos * kv, *v = L->v_cache + (size_t)pos * kv;
    float score[pos + 1];

    rmsnorm(h, x, &L->attn_norm, m->eps);
    matvec(q, &L->attn_q, h);
    matvec(k, &L->attn_k, h);
    matvec(v, &L->attn_v, h);
    for (int i = 0; i < m->n_head; i++) {
        rmsnorm(q + i * d, q + i * d, &L->attn_q_norm, m->eps);
        rope(q + i * d, d, pos, m->rope_base);
    }
    for (int i = 0; i < m->n_head_kv; i++) {
        rmsnorm(k + i * d, k + i * d, &L->attn_k_norm, m->eps);
        rope(k + i * d, d, pos, m->rope_base);
    }

    memset(sum, 0, sizeof sum);
    for (int i = 0; i < m->n_head; i++) {
        int off = i / (m->n_head / m->n_head_kv) * d;   /* its key head */
        for (int t = 0; t <= pos; t++)
            score[t] = dot(q + i * d, L->k_cache + (size_t)t * kv + off, d) / sqrtf(d);
        softmax(score, pos + 1);
        for (int t = 0; t <= pos; t++)
            for (int j = 0; j < d; j++)
                sum[i * d + j] += score[t] * L->v_cache[(size_t)t * kv + off + j];
    }
    matvec(y, &L->attn_output, sum);
    for (int i = 0; i < m->n_embd; i++)
        x[i] += y[i];
}

/* x += W_down (silu(W_gate x) * W_up x), with silu(a) = a / (1 + e^-a). */
static void feed_forward(struct model *m, int l, float *x)
{
    const struct layer *L = &m->layer[l];
    float h[m->n_embd], gate[m->n_ff], up[m->n_ff], y[m->n_embd];

    rmsnorm(h, x, &L->ffn_norm, m->eps);
    matvec(gate, &L->ffn_gate, h);
    matvec(up, &L->ffn_up, h);
    for (int i = 0; i < m->n_ff; i++)
        gate[i] = gate[i] / (1 + expf(-gate[i])) * up[i];
    matvec(y, &L->ffn_down, gate);
    for (int i = 0; i < m->n_embd; i++)
        x[i] += y[i];
}

/* The token whose row of the embedding matrix has the largest product with
   x: the embedding matrix is also the output layer of this model. */
static int next_token(struct model *m, const float *x)
{
    float h[m->n_embd];
    int best = 0;

    rmsnorm(h, x, &m->output_norm, m->eps);
    matvec(m->logits, &m->token_embd, h);
    for (int i = 1; i < m->n_vocab; i++)
        if (m->logits[i] > m->logits[best])
            best = i;
    return best;
}

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* The two rates carry the names `ollama run --verbose` gives them. A
   multiply-add counts as 2 operations, as in a roofline. */
static void report(int n_prompt, double t_prompt, int n_eval, double t_eval)
{
    fprintf(stderr, "\n");
    fprintf(stderr, "prompt eval rate:     %.2f tokens/s\n", n_prompt / t_prompt);
    fprintf(stderr, "eval rate:            %.2f tokens/s\n", n_eval / t_eval);
    fprintf(stderr, "matvec rate:          %.2f GFLOPS\n",
            2.0 * n_weights / (t_prompt + t_eval) / 1e9);
}

/* One pass of the loop takes the token at pos through the model. While the
   prompt lasts, the token after it is known; after that it is the one the
   model predicts. The first n - 1 passes are timed as "prompt eval", the
   others, each with a call of next_token(), as "eval". */
int main(int argc, char **argv)
{
    if (argc < 3)
        die("usage: mini-ollama MODEL.gguf PROMPT [MAX_NEW_TOKENS]");

    struct model *m = load_model(argv[1]);          /* the file ollama loads */
    int tok[MAX_CTX], pos;
    int n = tokenize(m, argv[2], tok);              /* the prompt as n tokens */
    float x[m->n_embd];                             /* the state of one token */
    int end = n + (argc > 3 ? atoi(argv[3]) : 64);
    if (end <= n)
        die("MAX_NEW_TOKENS has to be at least 1");
    if (end > MAX_CTX)
        end = MAX_CTX;

    double t0 = now(), t1 = t0;
    for (pos = 0; pos + 1 < end; pos++) {
        if (pos + 1 == n)
            t1 = now();
        embed(m, tok[pos], x);                      /* x = row tok[pos] of token_embd */
        for (int l = 0; l < m->n_layer; l++) {      /* 28 layers */
            attention(m, l, pos, x);                /* 4 matrix-vector products */
            feed_forward(m, l, x);                  /* 3 matrix-vector products */
        }
        if (pos + 1 < n)
            continue;                               /* the prompt gives the next token */
        tok[pos + 1] = next_token(m, x);            /* 1 matrix-vector product */
        if (tok[pos + 1] == m->eos)
            break;
        print_token(m, tok[pos + 1]);
    }
    report(n - 1, t1 - t0, pos + 1 - n + (pos + 1 < end), now() - t1);
    return 0;
}
