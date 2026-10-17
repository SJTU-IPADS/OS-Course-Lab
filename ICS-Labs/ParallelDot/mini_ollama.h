/* mini_ollama.h -- what the framework sources share. Finishing the lab does
   not need this file: the interface of your code is ops.h.

     main.c          the generation loop: one layer as calls of the operators
     model.c         reading the GGUF file
     tokenizer.c     text to token numbers and back
     selftest.c      checks a set of operators and times matvec; needs no model file
     ops/scalar.c    the operators, one element at a time, on the CPU
     ops/simd.c      the operators with AVX2 intrinsics, on the CPU
     ops/cuda.cu     the operators as CUDA kernels, on the GPU */
#ifndef MINI_OLLAMA_H
#define MINI_OLLAMA_H

#include <stddef.h>
#include <stdint.h>

enum { MAX_CTX = 512 };                 /* prompt and answer together, in tokens */

/* A string inside the file: not terminated, so it carries its length. */
struct str {
    const char *p;
    size_t      len;
};

/* A weight tensor inside the file. A matrix is stored one row after another,
   each row `cols` weights in Q4_0 (32 weights in 18 bytes). A vector has
   rows = 1 and is stored as floats. load_model() points data into the
   file; main.c points it to the copy in the memory of the operators. */
struct tensor {
    size_t         cols, rows;
    const uint8_t *data;
};

struct layer {
    struct tensor attn_norm, attn_q, attn_k, attn_v, attn_output;
    struct tensor attn_q_norm, attn_k_norm;
    struct tensor ffn_norm, ffn_gate, ffn_up, ffn_down;
    float *k_cache, *v_cache;           /* keys and values of the tokens so far;
                                           main.c allocates them */
};

struct model {
    int   n_layer, n_embd, n_ff, n_vocab;
    int   n_head, n_head_kv, head_dim;
    float eps, rope_base;

    struct tensor token_embd, output_norm;
    struct layer *layer;
    float        *logits;               /* n_vocab numbers, one per token;
                                           main.c allocates them */

    struct str *vocab;                  /* the text of each token */
    struct str *merges;                 /* "left right", the earliest merge first */
    int         n_merges;
    int         eos;                    /* the token that ends an answer */
    struct table *vocab_index, *merge_index;
};

/* model.c */
struct model *load_model(const char *path);
void          die(const char *fmt, ...);

/* tokenizer.c */
void tokenizer_init(struct model *m);
int  tokenize(const struct model *m, const char *prompt, int *tok);
void print_token(const struct model *m, int tok);

#endif
