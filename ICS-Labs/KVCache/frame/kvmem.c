/* kvmem.c -- the memory of the KV cache: one piece, allocated once and
   divided into blocks of one size, block tok at base + tok * block_bytes.
   The limit counts this memory alone. Every key and value of the cache is
   kept here, so the blocks reserved at the end are the memory the
   conversation needed. */
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "kvcache.h"
#include "mini_ollama.h"

void kv_memory_init(struct kv_cache *cache, size_t block_bytes, size_t limit)
{
    if (block_bytes == 0 || block_bytes % sizeof(float) != 0)
        die("kv_memory_init: a block of %zu bytes does not hold a whole number of floats, at least 1 "
            "(see kv_block_bytes)", block_bytes);
    size_t n_blocks = limit / block_bytes;
    if (n_blocks > INT_MAX)
        die("kv_memory_init: %zu bytes hold more than %d blocks of %zu bytes", limit, INT_MAX, block_bytes);

    size_t bytes = n_blocks * block_bytes;
    cache->base = malloc(bytes ? bytes : 1);
    if (!cache->base)
        die("out of memory for %zu blocks of %zu bytes", n_blocks, block_bytes);
    memset(cache->base, 0xFF, bytes);   /* NaN, so that a read of a block not written shows */
    cache->n_blocks = (int)n_blocks;
    cache->n_cached = 0;
    cache->capacity = 0;
}

void kv_memory_free(struct kv_cache *cache)
{
    free(cache->base);
    cache->base = NULL;
    cache->n_blocks = cache->n_cached = cache->capacity = 0;
}
