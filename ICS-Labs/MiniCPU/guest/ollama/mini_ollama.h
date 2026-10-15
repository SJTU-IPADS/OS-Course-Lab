/* mini_ollama.h -- what the source files share.

   The file of the same name of lecture 3 (lectures/3-asm/examples/
   mini-ollama), for a guest of mini-cpu. What differs: libc.h stands in for
   the standard headers, MAX_CTX is 128, load_model() takes the file from
   standard input, matvec() is declared here, and die() is in libc.c.

     mini_ollama.c   the generation loop and the arithmetic of one layer
     matvec.c        y = W x, one weight at a time
     matvec_simd.c   y = W x, 32 weights at a time, with dot_q4.s
     model.c         reading the GGUF file
     tokenizer.c     text to token numbers and back
     libc.c, math.c  the part of the C library the program uses */
#ifndef MINI_OLLAMA_H
#define MINI_OLLAMA_H

#include "libc.h"

/* Prompt and answer together, in tokens. The lecture has 512, whose key and
   value caches take 117 MB; the machine has 64 MB, and 128 takes 29 MB. */
enum { MAX_CTX = 128 };

/* A string inside the file: not terminated, so it carries its length. */
struct str {
    const char *p;
    size_t      len;
};

/* A weight tensor inside the file. A matrix is stored one row after another,
   each row `cols` weights in Q4_0 (32 weights in 18 bytes). A vector has
   rows = 1 and is stored as floats. */
struct tensor {
    size_t         cols, rows;
    const uint8_t *data;
};

struct layer {
    struct tensor attn_norm, attn_q, attn_k, attn_v, attn_output;
    struct tensor attn_q_norm, attn_k_norm;
    struct tensor ffn_norm, ffn_gate, ffn_up, ffn_down;
    float *k_cache, *v_cache;           /* keys and values of the tokens so far */
};

struct model {
    int   n_layer, n_embd, n_ff, n_vocab;
    int   n_head, n_head_kv, head_dim;
    float eps, rope_base;

    struct tensor token_embd, output_norm;
    struct layer *layer;
    float        *logits;               /* n_vocab numbers, one per token */

    struct str *vocab;                  /* the text of each token */
    struct str *merges;                 /* "left right", the earliest merge first */
    int         n_merges;
    int         eos;                    /* the token that ends an answer */
    struct table *vocab_index, *merge_index;
};

/* model.c */
struct model *load_model(void);
extern float  fp16[1 << 16];            /* fp16[h]: the value of the half-precision bits h */

/* matvec.c or matvec_simd.c */
void matvec(float *y, const struct tensor *w, const float *x);

/* tokenizer.c */
void tokenizer_init(struct model *m);
int  tokenize(const struct model *m, const char *prompt, int *tok);
void print_token(const struct model *m, int tok);

#endif
