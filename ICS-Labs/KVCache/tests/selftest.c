/* kvcache-selftest -- checks the functions you write in src/kvcache.c on a
   small cache, without a model: 3 layers, rows of 8 floats, so one token
   takes one block of 3 * 2 * 8 * 4 = 192 bytes. The memory of the cache is
   an array of this program, and base lies 12 bytes after its start: an
   address that is neither 0 nor a multiple of 8, so a key or a value is
   found only from base.

     ./kvcache-selftest

   The checks follow the order of src/kvcache.c, and each one calls only the
   functions before it: the fields a check needs are set by hand, as the
   functions before it would leave them. compute() is given, so no check
   calls it. One line per check, "pass" or "FAIL"; the exit status is the
   number of failed checks. A function that is not written yet stops the
   program at its first call. */
#include <stdio.h>
#include <string.h>

#include "kvcache.h"
#include "mini_ollama.h"

enum {
    LAYERS = 3, KV_DIM = 8,
    BLOCK_FLOATS = LAYERS * 2 * KV_DIM,
    BLOCK_BYTES = BLOCK_FLOATS * (int)sizeof(float),
    MEMORY_BLOCKS = 16,                 /* the most blocks a check gives the cache */
    BASE_OFFSET = 12                    /* bytes from the start of memory[] to base */
};

const char *program_name = "kvcache-selftest";

static _Alignas(64) float memory[MEMORY_BLOCKS * BLOCK_FLOATS + 16];   /* base: 12 bytes in */
static struct kv_cache cache;
static int fails;

static void verdict(int ok, const char *what)
{
    printf("  %s  %s\n", ok ? "pass" : "FAIL", what);
    if (!ok)
        fails++;
}

/* The cache of the checks: 3 layers, rows of 8 floats, n_blocks blocks
   from base, capacity of them reserved and n_cached of them cached. Every
   float of the memory holds its own index, so a change shows. */
static void reset(int n_blocks, int capacity, int n_cached)
{
    cache.n_layer = LAYERS;
    cache.kv_dim = KV_DIM;
    cache.base = (unsigned char *)memory + BASE_OFFSET;
    cache.n_blocks = n_blocks;
    cache.capacity = capacity;
    cache.n_cached = n_cached;
    for (size_t idx = 0; idx < sizeof memory / sizeof memory[0]; idx++)
        memory[idx] = (float)idx;
}

/* The key (kind 0) or the value (kind 1) of the token in the layer by the
   layout of kvcache.h, without kv_key() and kv_value(). */
static float *row_of(int layer, int kind, int tok)
{
    size_t block_bytes = (size_t)cache.n_layer * 2 * cache.kv_dim * sizeof(float);
    return (float *)(cache.base + tok * block_bytes) + (size_t)(2 * layer + kind) * cache.kv_dim;
}

/* ---------------- a state to compare with ---------------- */

static struct kv_cache saved;
static float saved_memory[sizeof memory / sizeof memory[0]];

static void save(void)
{
    saved = cache;
    memcpy(saved_memory, memory, sizeof memory);
}

/* Whether the fields of the framework are what save() saw. */
static int fields_stay(void)
{
    return cache.n_layer == saved.n_layer && cache.kv_dim == saved.kv_dim && cache.base == saved.base &&
           cache.n_blocks == saved.n_blocks;
}

/* Whether every byte of the memory is what save() saw. */
static int memory_stays(void)
{
    return memcmp(memory, saved_memory, sizeof memory) == 0;
}

static int nothing_changed(void)
{
    return fields_stay() && cache.n_cached == saved.n_cached && cache.capacity == saved.capacity &&
           memory_stays();
}

/* ---------------- what compute() calls ---------------- */

/* No check calls compute(): make test runs it with your functions on a
   whole conversation. It is linked in with src/kvcache.c, so the three
   functions of frame/main.c that it calls are defined here, empty. */
void embed(int token, float *state)
{
    (void)token; (void)state;
}

void attention(int layer, int pos, float *state, float *const *keys, float *const *values)
{
    (void)layer; (void)pos; (void)state; (void)keys; (void)values;
}

void feed_forward(int layer, float *state)
{
    (void)layer; (void)state;
}

/* ---------------- the checks ---------------- */

static void check_block_bytes(void)
{
    reset(0, 0, 0);
    int ok = kv_block_bytes(&cache) == BLOCK_BYTES;
    cache.n_layer = 28;
    cache.kv_dim = 1024;
    ok &= kv_block_bytes(&cache) == 229376;
    verdict(ok, "kv_block_bytes: 192 bytes here, 229376 bytes for the real model (28 layers, rows of 1024)");

    cache.n_layer = 4096;
    cache.kv_dim = 262144;
    verdict(kv_block_bytes(&cache) == (size_t)4096 * 2 * 262144 * 4,
            "kv_block_bytes: 4096 layers with rows of 262144 floats, beyond the largest int");
}

/* kv_key (kind 0) or kv_value (kind 1) for every layer and every block, on
   the cache of the checks and on one of 2 layers with rows of 4 floats
   whose base lies 40 bytes after the start of memory[]. */
static int rows_right(int kind)
{
    int ok = 1;
    for (int shape = 0; shape < 2; shape++) {
        reset(MEMORY_BLOCKS, MEMORY_BLOCKS, MEMORY_BLOCKS);
        if (shape == 1) {
            cache.n_layer = 2;
            cache.kv_dim = 4;
            cache.base = (unsigned char *)memory + 40;
        }
        for (int layer = 0; layer < cache.n_layer; layer++)
            for (int tok = 0; tok < cache.capacity; tok++)
                ok &= (kind ? kv_value(&cache, layer, tok) : kv_key(&cache, layer, tok)) ==
                      row_of(layer, kind, tok);
    }
    return ok;
}

static void check_key(void)
{
    verdict(rows_right(0), "kv_key: block tok lies tok blocks after base, then the layer's place in it");
}

static void check_value(void)
{
    verdict(rows_right(1), "kv_value: in the block of the token, right after the key of the same layer");
}

static void check_append(void)
{
    reset(MEMORY_BLOCKS, 4, 0);
    save();
    kv_append(&cache);
    kv_append(&cache);
    kv_append(&cache);
    verdict(cache.n_cached == 3 && cache.capacity == 4 && fields_stay() && memory_stays(),
            "kv_append: three calls, three tokens cached, nothing else changes");
}

static void check_reserve_first(void)
{
    reset(MEMORY_BLOCKS, 0, 0);
    save();
    int room = kv_reserve(&cache, 5, 1);
    verdict(room == 1 && cache.capacity == 5 && cache.n_cached == 0 && fields_stay() && memory_stays(),
            "kv_reserve, max_answer 1: the first turn gets blocks for what it can cache, no more");

    reset(MEMORY_BLOCKS, 0, 0);
    save();
    room = kv_reserve(&cache, 3, 4);
    verdict(room == 1 && cache.capacity == 6 && cache.n_cached == 0 && fields_stay() && memory_stays(),
            "kv_reserve, max_answer 4: the first turn gets blocks for what it can cache, no more");
}

static void check_reserve_grow(void)
{
    reset(MEMORY_BLOCKS, 5, 4);
    int room = kv_reserve(&cache, 5, 2);
    int ok = room == 1 && cache.capacity == 6;
    reset(MEMORY_BLOCKS, 5, 4);
    save();
    room = kv_reserve(&cache, 9, 4);
    verdict(ok && room == 1 && cache.capacity == 12,
            "kv_reserve, 4 tokens cached: a later turn gets blocks for what it can cache, no more");
    verdict(cache.n_cached == 4 && fields_stay() && memory_stays(),
            "kv_reserve: growing leaves the cached tokens where they are, nothing moved or copied");

    reset(MEMORY_BLOCKS, 12, 4);
    save();
    room = kv_reserve(&cache, 6, 2);
    int same = nothing_changed();
    room &= kv_reserve(&cache, 10, 3);
    same &= nothing_changed();
    verdict(room == 1 && same, "kv_reserve: a turn that fits in the capacity changes nothing");
}

static void check_reserve_full(void)
{
    reset(12, 5, 4);
    save();
    int room = kv_reserve(&cache, 9, 4);
    verdict(room == 1 && cache.capacity == 12 && cache.n_cached == 4 && fields_stay() && memory_stays(),
            "kv_reserve: the conversation can grow until it has every block of the cache");

    reset(12, 6, 6);
    save();
    room = kv_reserve(&cache, 10, 4);
    int same = nothing_changed();
    reset(12, 0, 0);
    save();
    room |= kv_reserve(&cache, 12, 2);
    same &= nothing_changed();
    reset(12, 0, 0);
    save();
    room |= kv_reserve(&cache, 13, 1);
    same &= nothing_changed();
    verdict(room == 0 && same,
            "kv_reserve: returns 0 and changes nothing for more tokens than the cache has blocks");
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);   /* keep the order with the messages on stderr */
    printf("kvcache.c on a cache of %d layers with rows of %d floats: %d bytes per token\n",
           LAYERS, KV_DIM, BLOCK_BYTES);
    check_block_bytes();
    check_key();
    check_value();
    check_append();
    check_reserve_first();
    check_reserve_grow();
    check_reserve_full();

    if (fails == 0)
        printf("all passed\n");
    else
        printf("%d failed\n", fails);
    return fails;
}
