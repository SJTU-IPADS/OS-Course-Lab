/* kvcache.h -- the KV cache of mini-ollama: what the framework
   (frame/main.c, frame/kvmem.c, tests/selftest.c) and your file
   src/kvcache.c share. Keep this file unchanged.

   mini-ollama answers the turns of one conversation. The cache keeps the
   key and the value of every token of the conversation computed so far, in
   every layer, so that a later step reads them instead of computing them
   again.

   The memory of the cache is one piece, allocated once by the framework
   and divided into blocks of one size. One block holds one token: its key
   and its value in every layer. The blocks follow one another in the order
   of the tokens, with nothing between them: the block of token tok starts
   at base + tok * (the bytes of one block).

     base
     |
     v
     +---------+---------+---------+---------+---------+---------+-----+---------+
     | block 0 | block 1 | block 2 | block 3 | block 4 | block 5 | ... |  last   |
     | token 0 | token 1 | token 2 | token 3 | token 4 | token 5 |     |         |
     +---------+---------+---------+---------+---------+---------+-----+---------+
     |<---------- cached --------->|<---- reserved --->|<----- not used yet ---->|
     0                             n_cached 3          capacity 5         n_blocks

     one block
     +-------------+-------------+-------------+-----+--------------------+
     | layer 0     | layer 0     | layer 1     |     | layer n_layer-1    |
     | key         | value       | key         | ... | value              |
     +-------------+-------------+-------------+-----+--------------------+
       kv_dim floats each, in this order, with nothing between them

   Blocks 0 .. n_cached - 1 hold the tokens computed so far. Blocks
   n_cached .. capacity - 1 are reserved for the next tokens of the turn:
   what they contain has no meaning yet. The blocks from capacity on are
   not used yet. A turn that needs more blocks than the memory has does not
   fit, and the program stops. */
#ifndef KVCACHE_H
#define KVCACHE_H

#include <stddef.h>

enum {
    KV_MAX_TOKENS = 512     /* tokens of the conversation, at most: the
                               framework stops at a longer text */
};

struct kv_cache {
    int            n_layer;     /* layers of the model */
    int            kv_dim;      /* floats in one key, and in one value, of a
                                   token in one layer: n_head_kv * head_dim */
    unsigned char *base;        /* the address of the first byte of block 0,
                                   set by the framework */
    int            n_blocks;    /* blocks in the memory of the cache: the
                                   whole blocks that fit in the limit */
    int            n_cached;    /* blocks 0 .. n_cached - 1 hold computed
                                   tokens */
    int            capacity;    /* blocks 0 .. capacity - 1 are reserved; 0
                                   before the first turn, at most n_blocks */
};

/* -------- src/kvcache.c: given, written out as an example -------- */

/* Takes the token at position n_cached through all layers, with the keys
   and values of the tokens before it. Afterwards state is the vector of the
   token after the last layer, and the token is cached. Written out in
   src/kvcache.c as an example of the flow, and of how kv_key(), kv_value()
   and kv_append() are used. */
void compute(struct kv_cache *cache, int token, float *state);

/* -------- src/kvcache.c: your functions, in the order you write them -------- */

/* The bytes of one block: the keys and the values of one token in all
   layers. */
size_t kv_block_bytes(const struct kv_cache *cache);

/* The key (the value) of token tok in the layer: the address of the first
   of its kv_dim floats, in the block of that token. */
float *kv_key(const struct kv_cache *cache, int layer, int tok);
float *kv_value(const struct kv_cache *cache, int layer, int tok);

/* Records that one more token is cached: attention() has written its key
   and value in every layer into block n_cached. */
void kv_append(struct kv_cache *cache);

/* Makes room for a turn, whose text with the message has n_prompt tokens
   (the earlier turns included) and whose answer has at most max_answer
   tokens, at least 1: works out how many tokens the conversation can have
   cached at the end of the turn and reserves a block for each. The blocks
   of the cached tokens stay where they are. Returns 1, or 0, with nothing
   changed, when the memory of the cache has too few blocks. */
int kv_reserve(struct kv_cache *cache, int n_prompt, int max_answer);

/* -------- frame/main.c: what compute() calls (framework) -------- */

/* state, in all three, is the vector of one token: compute() passes on the
   one it is given. The weights are those of the model, which the framework
   holds. */

/* Reads the row of the token in the embedding matrix. Writes state: that
   row, decoded. */
void embed(int token, float *state);

/* The attention of the layer, for the token at position pos. keys[tok] and
   values[tok], for tok = 0 .. pos, are the addresses of the key and of the
   value of token tok in this layer.
     reads   state, the input of the layer; keys[0 .. pos] and
             values[0 .. pos]; the weights of the layer
     writes  *keys[pos] and *values[pos], kv_dim floats each: the key and
             the value of this token, before it reads them; state, to which
             it adds what the token takes from the tokens 0 .. pos
   It first checks that each address lies in blocks 0 .. capacity - 1 of
   the cache, and stops the program when one does not. */
void attention(int layer, int pos, float *state, float *const *keys, float *const *values);

/* The feed-forward network of the layer.
     reads   state, as attention() left it; the weights of the layer
     writes  state, to which it adds the output of the network */
void feed_forward(int layer, float *state);

#endif /* KVCACHE_H */
