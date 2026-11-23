/* kvcache.c -- the KV cache of mini-ollama, and compute(), which takes one
 * token through the model with it. All the code you write goes in this
 * file. compute, the first function after the helper todo(), is given as
 * an example: read it first. In each of the five functions after compute,
 * replace the body (the (void) line, the todo() call and the return, where
 * there is one) with your implementation. kvcache.h describes struct
 * kv_cache, the memory of the cache with its blocks, the layout of one
 * block, and the functions of frame/main.c that compute() calls.
 *
 *   make                  build mini-ollama and kvcache-selftest
 *   ./kvcache-selftest    check each function you write on a small cache,
 *                         in the order of this file
 *   make test             the same, then a whole conversation on a small
 *                         model, compared with --no-cache
 *
 * Given, written out as an example:
 *   compute                        one token through the model, and how it
 *                                  uses the functions you write
 *
 * The functions you write, in the order of this file:
 *   kv_block_bytes                 how large one block is
 *   kv_key, kv_value               where the key and the value of a token are
 *   kv_append                      how many tokens are cached
 *   kv_reserve                     the blocks of a turn
 *
 * How frame/main.c calls them:
 *   at the start, the memory of the cache is divided into blocks of
 *   kv_block_bytes(cache);
 *   in each turn of the conversation (serve):
 *     kv_reserve(cache, n_prompt, max_answer);
 *     for each token that goes into the model as input, in the order of the
 *     text, from token n_cached on:
 *         compute(cache, the token, state);
 *             which calls kv_key() and kv_value() in each layer, and
 *             kv_append() after the last one.
 */
#include <stdio.h>
#include <stdlib.h>

#include "kvcache.h"
#include "mini_ollama.h"

/* A function that is not written yet calls todo("name"): the program then
 * reports the name and exits with status 3. The attribute keeps the
 * compiler from reporting todo once no function calls it. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline void todo(const char *name)
{
    fprintf(stderr, "%s is not implemented yet\n", name);
    exit(3);
}

/* One token through the model, with the KV cache. This function is given:
 * it is written out as an example of the whole flow, and of how the
 * functions you write are used. It calls kv_key() and kv_value() in every
 * layer and kv_append() at the end. You do not need to change it.
 *
 * frame/main.c calls it (compute_token) after kv_reserve() has started the
 * turn: once for each token that goes into the model as input, in the order
 * of the text of the conversation, starting at the first token that is not
 * cached. n_cached is then smaller than capacity, since kv_reserve() gave
 * the token a block.
 *
 *   token   the number of the token in the vocabulary
 *   state   the vector of the token on its way through the layers. The
 *           memory belongs to frame/main.c, and embed(), attention() and
 *           feed_forward() change it in place.
 *
 * When it returns, state is the vector after the last layer, from which
 * next_token() of frame/main.c predicts the token that follows, and the
 * token is cached.
 */
void compute(struct kv_cache *cache, int token, float *state)
{
    float *keys[KV_MAX_TOKENS], *values[KV_MAX_TOKENS];

    /* 1. The tokens 0 .. n_cached - 1 are cached and this token is the next
     *    one of the text, so its position pos is n_cached as it is at the
     *    call. The position is the same in every layer. */
    int pos = cache->n_cached;

    /* 2. embed() turns the number of the token into state, the input of
     *    layer 0. */
    embed(token, state);

    for (int layer = 0; layer < cache->n_layer; layer++) {
        /* 3. Where the key and the value of each token 0 .. pos are in this
         *    layer, entry tok for token tok: kv_key() and kv_value() work
         *    them out from the number of the token. The block of token pos
         *    is still a reserved one, which attention() fills in step 4. */
        for (int tok = 0; tok <= pos; tok++) {
            keys[tok] = kv_key(cache, layer, tok);
            values[tok] = kv_value(cache, layer, tok);
        }

        /* 4. attention() first writes the key and the value of this token
         *    where keys[pos] and values[pos] point. Then it reads the keys
         *    and values of the tokens 0 .. pos and adds what this token
         *    takes from them to state. Those of the tokens before pos were
         *    written when compute() took these tokens through the model:
         *    this call reads them from the cache. */
        attention(layer, pos, state, keys, values);

        /* 5. feed_forward() updates state. What it leaves is the input of
         *    the next layer. */
        feed_forward(layer, state);
    }

    /* 6. The block of the token now holds its key and its value in every
     *    layer: kv_append() counts the token as cached, once for the token
     *    and only after the last layer. */
    kv_append(cache);
}

/* The bytes of one block. A block holds one token (the figure in
 * kvcache.h): in each of the n_layer layers one key and one value, each
 * kv_dim floats, and nothing else. frame/main.c divides the memory of the
 * cache into blocks of this size.
 *
 * TODO: return that product, in bytes. Compute it in size_t: with many
 * layers and long keys it goes beyond the largest int.
 */
size_t kv_block_bytes(const struct kv_cache *cache)
{
    (void)cache;
    todo("kv_block_bytes");
    return 0;
}

/* The key of token tok in the layer: the address of the first of its
 * kv_dim floats. The blocks follow one another from base in the order of
 * the tokens, so the place of a token follows from its number alone.
 *
 * TODO: find the start of the block of token tok, tok blocks after base.
 * base is the address of a byte and kv_block_bytes() counts bytes. In front
 * of the key of the layer lie, in that block, the key and the value of each
 * of the layers 0 .. layer - 1. Return the start of the block plus that
 * many floats. The layer is 0 .. n_layer - 1 and tok is 0 .. capacity - 1:
 * a reserved block has addresses as well, attention() writes there.
 */
float *kv_key(const struct kv_cache *cache, int layer, int tok)
{
    (void)cache; (void)layer; (void)tok;
    todo("kv_key");
    return NULL;
}

/* The value of token tok in the layer: in the same block, right after the
 * key of the same layer.
 *
 * TODO: return kv_key() of the same layer and token plus kv_dim floats.
 */
float *kv_value(const struct kv_cache *cache, int layer, int tok)
{
    (void)cache; (void)layer; (void)tok;
    todo("kv_value");
    return NULL;
}

/* The key and the value of token n_cached are in block n_cached, where
 * attention() wrote them in every layer: that token is cached now.
 *
 * TODO: add 1 to n_cached. compute() calls this only while n_cached is
 * smaller than capacity: kv_reserve() gave the token its block before.
 */
void kv_append(struct kv_cache *cache)
{
    (void)cache;
    todo("kv_append");
}

/* Makes room for a turn: afterwards the conversation has a block for every
 * token that can be cached in this turn. frame/main.c calls it once per
 * turn, when the message has joined the text of the conversation. Returns
 * 1 when the room is there, 0 when the turn does not fit in the memory of
 * the cache.
 *
 *   n_prompt     the tokens of the text with the message of this turn, the
 *                text of the earlier turns included
 *   max_answer   the most tokens the answer of this turn can have, at
 *                least 1
 *
 * TODO, in this order:
 *   1. Work out n_tokens, the most tokens the conversation can have cached
 *      at the end of this turn. It follows from three facts:
 *      - a token is cached when compute() takes it through the model as
 *        input;
 *      - each token of the answer is an output, predicted from the state
 *        that compute() left for the token before it;
 *      - the answer stops when its token number max_answer is generated,
 *        or earlier at the end token of the model.
 *      Reserve exactly the blocks this turn can use.
 *   2. If capacity is n_tokens or more, the room is there: return 1 and
 *      change nothing.
 *   3. If n_tokens is more than n_blocks, the turn does not fit: return 0
 *      and change nothing.
 *   4. Set capacity to n_tokens and return 1. n_cached stays as it is, and
 *      so does the memory: the blocks capacity .. n_tokens - 1 follow those
 *      before them, so nothing is allocated, moved or copied.
 * Easy to get wrong:
 *   - n_tokens equal to n_blocks fits: the turn then uses every block;
 *   - a turn that does not fit leaves capacity as it was.
 * make test compares the capacity after every call with these rules.
 */
int kv_reserve(struct kv_cache *cache, int n_prompt, int max_answer)
{
    (void)cache; (void)n_prompt; (void)max_answer;
    todo("kv_reserve");
    return 0;
}
