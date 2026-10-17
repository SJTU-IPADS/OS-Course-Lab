/* mini-ollama: generate text from a quantized model.

     make
     ./mini-ollama-simd MODEL.gguf PROMPT.txt [MAX_NEW_TOKENS]

   MODEL.gguf is q4_0.gguf (README.md), the file the nano-quant lab builds
   with --recipe q4_0 and ollama loads: the language model of
   Qwen3-VL-2B-Instruct, every matrix in Q4_0. PROMPT.txt holds the question, UTF-8. The answer goes to standard
   output as it is generated, the timing to standard error, under the names
   `ollama run --verbose` prints.

   Every computation is a call of an operator of ops.h. The set of
   operators is chosen when the program is built: the Makefile compiles
   this file with -DOPS=ops_scalar for mini-ollama-scalar, -DOPS=ops_simd
   for mini-ollama-simd and -DOPS=ops_cuda for mini-ollama-cuda.
   ops/scalar.c is the lecture's mini_ollama one element at a time on one
   thread of the CPU. Before the first token the weights are copied into
   the memory of the operators (GPU memory for ops/cuda.cu; the CPU sets
   use them where they are), and every vector of the computation lives
   there too; per token the program only receives the number of the next
   token. A generated token takes 197 products of a matrix with a vector
   (7 in each of the 28 layers and one for the output), so nearly all the
   time is spent in the operator matvec. The next token is the one with the
   largest score, which is what ollama does at temperature 0. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mini_ollama.h"
#include "ops.h"

#ifndef OPS
#error "OPS names the set of operators: build with make, which gives -DOPS=ops_simd and so on"
#endif

static const struct ops *op = &OPS;
static uint64_t n_weights;                  /* weights multiplied so far */

/* The vectors of one pass through the model, in the memory of the
   operators. */
static struct {
    float *x;                   /* n_embd: the state of the token */
    float *h;                   /* n_embd: the state normalized */
    float *q, *sum;             /* n_head * head_dim: queries, what the heads take */
    float *score;               /* n_head * MAX_CTX: row h, the scores of head h */
    float *y;                   /* n_embd: what a layer adds to x */
    float *gate, *up;           /* n_ff */
} a;

/* The bytes of a tensor in the file: a matrix is rows * cols / 32 blocks
   of 18 bytes, a vector cols floats. */
static size_t bytes(const struct tensor *t)
{
    return t->rows > 1 ? t->rows * (t->cols / Q4_0_BLOCK_WEIGHTS) * Q4_0_BLOCK_BYTES
                       : t->cols * sizeof(float);
}

static void upload(struct tensor *t)
{
    t->data = op->upload(t->data, bytes(t));
}

static float *alloc(size_t n)
{
    return op->alloc(n * sizeof(float));
}

/* Every weight into the memory of the operators, and room for the caches,
   the scores and the vectors. */
static void place(struct model *m)
{
    size_t kv = (size_t)m->n_head_kv * m->head_dim, q = (size_t)m->n_head * m->head_dim;

    upload(&m->token_embd);
    upload(&m->output_norm);
    for (int l = 0; l < m->n_layer; l++) {
        struct layer *L = &m->layer[l];
        struct tensor *t[] = { &L->attn_norm, &L->attn_q, &L->attn_k, &L->attn_v,
                               &L->attn_output, &L->attn_q_norm, &L->attn_k_norm,
                               &L->ffn_norm, &L->ffn_gate, &L->ffn_up, &L->ffn_down };
        for (size_t i = 0; i < sizeof t / sizeof t[0]; i++)
            upload(t[i]);
        L->k_cache = alloc(MAX_CTX * kv);
        L->v_cache = alloc(MAX_CTX * kv);
    }
    m->logits = alloc(m->n_vocab);
    a.x = alloc(m->n_embd);
    a.h = alloc(m->n_embd);
    a.q = alloc(q);
    a.sum = alloc(q);
    a.score = alloc(m->n_head * (size_t)MAX_CTX);
    a.y = alloc(m->n_embd);
    a.gate = alloc(m->n_ff);
    a.up = alloc(m->n_ff);
}

/* y = W x, and the count of the weights for the matvec rate. */
static void matvec(float *y, const struct tensor *w, const float *x)
{
    op->matvec(y, w->data, x, w->rows, w->cols);
    n_weights += w->rows * w->cols;
}

/* The floats of a vector tensor: the gains of a normalization. */
static const float *gain(const struct tensor *g)
{
    return (const float *)g->data;
}

/* x = row tok of the embedding matrix. A row of cols weights is cols / 32
   blocks of 18 bytes. */
static void embed(const struct model *m, int tok, float *x)
{
    const struct tensor *w = &m->token_embd;
    op->embed(x, w->data + (size_t)tok * (w->cols / Q4_0_BLOCK_WEIGHTS * Q4_0_BLOCK_BYTES), w->cols);
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
    float *k = L->k_cache + (size_t)pos * kv, *v = L->v_cache + (size_t)pos * kv;

    op->rmsnorm(a.h, x, gain(&L->attn_norm), 1, m->n_embd, m->eps);
    matvec(a.q, &L->attn_q, a.h);
    matvec(k, &L->attn_k, a.h);                     /* into the cache */
    matvec(v, &L->attn_v, a.h);
    op->rmsnorm(a.q, a.q, gain(&L->attn_q_norm), m->n_head, d, m->eps);
    op->rope(a.q, m->n_head, d, pos, m->rope_base);
    op->rmsnorm(k, k, gain(&L->attn_k_norm), m->n_head_kv, d, m->eps);
    op->rope(k, m->n_head_kv, d, pos, m->rope_base);

    op->attn_scores(a.score, a.q, L->k_cache, m->n_head, m->n_head_kv, d, pos + 1);
    op->softmax(a.score, m->n_head, pos + 1);
    op->attn_values(a.sum, a.score, L->v_cache, m->n_head, m->n_head_kv, d, pos + 1);
    matvec(a.y, &L->attn_output, a.sum);
    op->add(x, a.y, m->n_embd);
}

/* x += W_down (silu(W_gate x) * W_up x), with silu(a) = a / (1 + e^-a). */
static void feed_forward(struct model *m, int l, float *x)
{
    const struct layer *L = &m->layer[l];

    op->rmsnorm(a.h, x, gain(&L->ffn_norm), 1, m->n_embd, m->eps);
    matvec(a.gate, &L->ffn_gate, a.h);
    matvec(a.up, &L->ffn_up, a.h);
    op->swiglu(a.gate, a.up, m->n_ff);
    matvec(a.y, &L->ffn_down, a.gate);
    op->add(x, a.y, m->n_embd);
}

/* The token whose row of the embedding matrix has the largest product with
   x: the embedding matrix is also the output layer of this model. */
static int next_token(struct model *m, const float *x)
{
    op->rmsnorm(a.h, x, gain(&m->output_norm), 1, m->n_embd, m->eps);
    matvec(m->logits, &m->token_embd, a.h);
    return op->argmax(m->logits, m->n_vocab);
}

static double now(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* The two rates carry the names `ollama run --verbose` gives them. A
   multiply-add counts as 2 operations, as in a roofline. A Q4_0 weight
   takes 18 / 32 bytes; the weights are read once per token, and nearly all
   the bytes the operators read are weights. */
static void report(const char *machine, int n_prompt, double t_prompt, int n_eval, double t_eval)
{
    double t = t_prompt + t_eval;
    fprintf(stderr, "\n");
    fprintf(stderr, "operators:            %s\n", op->name);
    fprintf(stderr, "device:               %s\n", machine);
    fprintf(stderr, "prompt eval rate:     %.2f tokens/s\n", n_prompt / t_prompt);
    fprintf(stderr, "eval rate:            %.2f tokens/s\n", n_eval / t_eval);
    fprintf(stderr, "matvec rate:          %.2f GFLOPS\n", 2.0 * n_weights / t / 1e9);
    fprintf(stderr, "weight bandwidth:     %.1f GB/s\n",
            n_weights * (double)Q4_0_BLOCK_BYTES / Q4_0_BLOCK_WEIGHTS / t / 1e9);
}

/* The contents of the prompt file, without the line end at its end. A file
   keeps the question in UTF-8 on every system; a command-line argument does
   not on Windows. */
static char *read_prompt(const char *path)
{
    FILE *f = fopen(path, "rb");
    char *text = malloc(MAX_CTX * 4 + 1);
    if (!f)
        die("cannot open %s", path);
    if (!text)
        die("out of memory");
    size_t n = fread(text, 1, MAX_CTX * 4, f);
    fclose(f);
    while (n > 0 && (text[n - 1] == '\n' || text[n - 1] == '\r'))
        n--;
    if (n == 0)
        die("%s holds no prompt", path);
    text[n] = 0;
    return text;
}

/* One pass of the loop takes the token at pos through the model. While the
   prompt lasts, the token after it is known; after that it is the one the
   model predicts. The first n - 1 passes are timed as "prompt eval", the
   others, each with a call of next_token(), as "eval". A kernel launch
   returns before the kernel has run, so the clock is read after finish();
   argmax waits for its kernel anyway. */
int main(int argc, char **argv)
{
    if (argc < 3)
        die("usage: %s MODEL.gguf PROMPT.txt [MAX_NEW_TOKENS]", argv[0]);

    const char *machine = op->init();
    struct model *m = load_model(argv[1]);          /* the file ollama loads */
    char *prompt = read_prompt(argv[2]);
    int tok[MAX_CTX], pos;
    int n = tokenize(m, prompt, tok);               /* the prompt as n tokens */
    int end = n + (argc > 3 ? atoi(argv[3]) : 64);
    if (end <= n)
        die("MAX_NEW_TOKENS has to be at least 1");
    if (end > MAX_CTX)
        end = MAX_CTX;
    place(m);
    op->finish();

    double t0 = now(), t1 = t0;
    for (pos = 0; pos + 1 < end; pos++) {
        if (pos + 1 == n) {
            op->finish();
            t1 = now();
        }
        embed(m, tok[pos], a.x);                    /* x = row tok[pos] of token_embd */
        for (int l = 0; l < m->n_layer; l++) {      /* 28 layers */
            attention(m, l, pos, a.x);              /* 4 matrix-vector products */
            feed_forward(m, l, a.x);                /* 3 matrix-vector products */
        }
        if (pos + 1 < n)
            continue;                               /* the prompt gives the next token */
        tok[pos + 1] = next_token(m, a.x);          /* 1 matrix-vector product */
        if (tok[pos + 1] == m->eos)
            break;
        print_token(m, tok[pos + 1]);
    }
    op->finish();
    report(machine, n - 1, t1 - t0, pos + 1 - n + (pos + 1 < end), now() - t1);
    return 0;
}
