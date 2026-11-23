/* mini-ollama: generate text from a quantized model, as a guest of mini-cpu.

     make guest/mini-ollama guest/mini-ollama-simd
     ./mini-cpu guest/mini-ollama PROMPT [MAX_NEW_TOKENS] < MODEL.gguf

   The model comes on standard input, and libc.h stands in for the
   standard headers. matvec() has a file of its own in two versions,
   matvec.c and matvec_simd.c. The report at the end has counts and a
   checksum and no rates: the machine has no clock.

   MODEL.gguf is q4_0.gguf, a file ollama loads as well: the language model
   of Qwen3-VL-2B-Instruct, every matrix in Q4_0. guest/tiny.gguf (tools/mkmodel.py) is a model of two layers in the
   same format. The answer goes to standard output as it is generated, the
   report to standard error.

   Every step is written in the plainest way. A generated token takes 197
   products of a matrix with a vector (7 in each of the 28 layers and one for
   the output), so nearly all the time is spent in matvec(). The next token
   is the one with the largest score, which is what ollama does at
   temperature 0. */
#include "libc.h"
#include "mini_ollama.h"

/* state = row tok of the embedding matrix. */
static void embed(const struct model *model, int tok, float *state)
{
    const struct tensor *matrix = &model->token_embd;
    const uint8_t *blk = matrix->data + (size_t)tok * (matrix->cols / 32 * 18);
    for (size_t col = 0; col < matrix->cols; col += 32, blk += 18) {
        float scale = fp16[blk[0] | blk[1] << 8];
        for (int byte = 0; byte < 16; byte++) {
            state[col + byte]      = scale * ((blk[2 + byte] & 15) - 8);
            state[col + byte + 16] = scale * ((blk[2 + byte] >> 4) - 8);
        }
    }
}

/* out = in scaled to root mean square 1, times the gains in norm. out and in
   may be the same vector. */
static void rmsnorm(float *out, const float *in, const struct tensor *norm,
                    float eps)
{
    const float *gain = (const float *)norm->data;
    float ss = 0;
    for (size_t idx = 0; idx < norm->cols; idx++)
        ss += in[idx] * in[idx];
    float scale = 1 / sqrtf(ss / norm->cols + eps);
    for (size_t idx = 0; idx < norm->cols; idx++)
        out[idx] = in[idx] * scale * gain[idx];
}

/* Mark vec with its position: pair i, (vec[i], vec[i + dim/2]), turns by the
   angle pos * base^(-2i/dim). */
static void rope(float *vec, int dim, int pos, float base)
{
    for (int pair = 0; pair < dim / 2; pair++) {
        float angle = pos * powf(base, -2.0f * pair / dim);
        float cosine = cosf(angle), sine = sinf(angle);
        float first = vec[pair], second = vec[pair + dim / 2];
        vec[pair]           = first * cosine - second * sine;
        vec[pair + dim / 2] = second * cosine + first * sine;
    }
}

static float dot(const float *first, const float *second, int len)
{
    float sum = 0;
    for (int idx = 0; idx < len; idx++)
        sum += first[idx] * second[idx];
    return sum;
}

/* Turn scores[0..count) into weights that are positive and add up to 1. */
static void softmax(float *scores, int count)
{
    float max = scores[0], sum = 0;
    for (int idx = 1; idx < count; idx++)
        if (scores[idx] > max)
            max = scores[idx];
    for (int idx = 0; idx < count; idx++)
        sum += scores[idx] = expf(scores[idx] - max);
    for (int idx = 0; idx < count; idx++)
        scores[idx] /= sum;
}

/* state += what the token at pos takes from the tokens up to pos.

   The token asks with a query; every token so far answers with its key and
   value, kept in the cache of this layer. A head adds up the values, each
   weighted by how well its key matches the query. There are n_head query
   heads and n_head_kv key heads: n_head / n_head_kv query heads share one
   key head. */
static void attention(struct model *model, int layer_idx, int pos, float *state)
{
    struct layer *layer = &model->layer[layer_idx];
    int head_dim = model->head_dim, kv = model->n_head_kv * head_dim;
    float normed[model->n_embd], query[model->n_head * head_dim],
          sum[model->n_head * head_dim], out[model->n_embd];
    float *key   = layer->k_cache + (size_t)pos * kv,
          *value = layer->v_cache + (size_t)pos * kv;
    float score[pos + 1];

    rmsnorm(normed, state, &layer->attn_norm, model->eps);
    matvec(query, &layer->attn_q, normed);
    matvec(key, &layer->attn_k, normed);
    matvec(value, &layer->attn_v, normed);
    for (int head = 0; head < model->n_head; head++) {
        rmsnorm(query + head * head_dim, query + head * head_dim,
                &layer->attn_q_norm, model->eps);
        rope(query + head * head_dim, head_dim, pos, model->rope_base);
    }
    for (int head = 0; head < model->n_head_kv; head++) {
        rmsnorm(key + head * head_dim, key + head * head_dim,
                &layer->attn_k_norm, model->eps);
        rope(key + head * head_dim, head_dim, pos, model->rope_base);
    }

    memset(sum, 0, sizeof sum);
    for (int head = 0; head < model->n_head; head++) {
        /* where its key head starts in a row of the cache */
        int off = head / (model->n_head / model->n_head_kv) * head_dim;
        for (int token = 0; token <= pos; token++)
            score[token] = dot(query + head * head_dim,
                               layer->k_cache + (size_t)token * kv + off,
                               head_dim) / sqrtf(head_dim);
        softmax(score, pos + 1);
        for (int token = 0; token <= pos; token++)
            for (int idx = 0; idx < head_dim; idx++)
                sum[head * head_dim + idx] +=
                    score[token] * layer->v_cache[(size_t)token * kv + off + idx];
    }
    matvec(out, &layer->attn_output, sum);
    for (int idx = 0; idx < model->n_embd; idx++)
        state[idx] += out[idx];
}

/* state += W_down (silu(W_gate state) * W_up state), with
   silu(a) = a / (1 + e^-a). */
static void feed_forward(struct model *model, int layer_idx, float *state)
{
    const struct layer *layer = &model->layer[layer_idx];
    float normed[model->n_embd], gate[model->n_ff], up[model->n_ff],
          out[model->n_embd];

    rmsnorm(normed, state, &layer->ffn_norm, model->eps);
    matvec(gate, &layer->ffn_gate, normed);
    matvec(up, &layer->ffn_up, normed);
    for (int idx = 0; idx < model->n_ff; idx++)
        gate[idx] = gate[idx] / (1 + expf(-gate[idx])) * up[idx];
    matvec(out, &layer->ffn_down, gate);
    for (int idx = 0; idx < model->n_embd; idx++)
        state[idx] += out[idx];
}

static uint32_t check;                  /* over every score computed so far */

/* The token whose row of the embedding matrix has the largest product with
   state: the embedding matrix is also the output layer of this model. The
   bits of all n_vocab scores go into the checksum (FNV-1a, a float at a
   time). */
static int next_token(struct model *model, const float *state)
{
    float normed[model->n_embd];
    int best = 0;

    rmsnorm(normed, state, &model->output_norm, model->eps);
    matvec(model->logits, &model->token_embd, normed);
    for (int token = 1; token < model->n_vocab; token++)
        if (model->logits[token] > model->logits[best])
            best = token;
    for (int token = 0; token < model->n_vocab; token++) {
        union { float value; uint32_t bits; } score = { model->logits[token] };
        check = (check ^ score.bits) * 16777619u;
    }
    return best;
}

/* What the run did, on standard error. The checksum covers the scores of
   every token of every generated position, and those depend on every
   number computed before them: two runs that print the same checksum have
   computed the same numbers at every step. */
static void report(int n_prompt, int n_made)
{
    char line[128];
    int len = snprintf(line, sizeof line,
                       "prompt tokens:    %d\ngenerated tokens: %d\nlogits checksum:  %u\n",
                       n_prompt, n_made, (unsigned)check);
    write(2, line, (size_t)len);
}

/* One pass of the loop takes the token at pos through the model. While the
   prompt lasts, the token after it is known; after that it is the one the
   model predicts. */
int guest_main(int argc, char **argv)
{
    if (argc < 2)
        die("usage: mini-ollama PROMPT [MAX_NEW_TOKENS] < MODEL.gguf");

    struct model *model = load_model();             /* the file ollama loads */
    int tok[MAX_CTX], pos, made = 0;                /* made: tokens the model predicted */
    int n_prompt = tokenize(model, argv[1], tok);   /* the prompt as n_prompt tokens */
    float state[model->n_embd];                     /* the state of one token */
    int end = n_prompt + (argc > 2 ? atoi(argv[2]) : 64);
    if (end <= n_prompt)
        die("MAX_NEW_TOKENS has to be at least 1");
    if (end > MAX_CTX)
        end = MAX_CTX;

    for (pos = 0; pos + 1 < end; pos++) {
        embed(model, tok[pos], state);              /* row tok[pos] of token_embd */
        for (int layer = 0; layer < model->n_layer; layer++) {  /* 28 layers */
            attention(model, layer, pos, state);    /* 4 matrix-vector products */
            feed_forward(model, layer, state);      /* 3 matrix-vector products */
        }
        if (pos + 1 < n_prompt)
            continue;                               /* the prompt gives the next token */
        tok[pos + 1] = next_token(model, state);    /* 1 matrix-vector product */
        made++;
        if (tok[pos + 1] == model->eos)
            break;                                  /* the end token is not printed */
        print_token(model, tok[pos + 1]);
    }
    putchar('\n');
    flush();
    report(n_prompt, made);
    return 0;
}
