/* mini_ollama.h -- what the framework sources share. Finishing the lab does
   not need this file: the interface of your code is kvcache.h.

     frame/main.c        the turns of the workload; embed, attention and
                         feed_forward as calls of the operators; the run
                         without a cache
     frame/model.c       reading the GGUF file
     frame/tokenizer.c   text to token numbers and back
     frame/ops.c         the operators, one element at a time
     frame/kvmem.c       the memory of the KV cache, one piece divided into
                         blocks
     src/kvcache.c       your file: the functions of the KV cache that
                         you write, and compute, given as an example, which
                         takes a token through the model
     tests/selftest.c    checks the functions of src/kvcache.c without a
                         model */
#ifndef MINI_OLLAMA_H
#define MINI_OLLAMA_H

#include <stddef.h>
#include <stdint.h>

/* A string inside the file: not terminated, so it carries its length. */
struct str {
    const char *text;
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
};

struct model {
    int   n_layer, n_embd, n_ff, n_vocab;
    int   n_head, n_head_kv, head_dim;
    float eps, rope_base;

    struct tensor token_embd, output_norm;
    struct layer *layer;
    size_t        file_bytes;           /* the model file, read into memory */

    struct str *vocab;                  /* the text of each token */
    struct str *merges;                 /* "left right", the earliest merge first */
    int         n_merges;
    int         eos;                    /* the token that ends an answer */
    struct table *vocab_index, *merge_index;
};

/* frame/model.c */
struct model *load_model(const char *path);
void          die(const char *fmt, ...);
extern const char *program_name;        /* in front of the messages of die();
                                           each program defines it */

/* frame/kvmem.c: kv_memory_init() allocates the memory of the cache, as
   many whole blocks of block_bytes as fit in limit bytes, every byte 0xFF,
   so that a float read before it is written is NaN; it sets base and
   n_blocks, and n_cached and capacity to 0. kv_memory_free() gives the
   memory back. */
struct kv_cache;
void kv_memory_init(struct kv_cache *cache, size_t block_bytes, size_t limit);
void kv_memory_free(struct kv_cache *cache);

/* frame/tokenizer.c */
void tokenizer_init(struct model *model);
int  tokenize(const struct model *model, const char *text, int *tokens, int room);
void print_token(const struct model *model, int token);

#endif
