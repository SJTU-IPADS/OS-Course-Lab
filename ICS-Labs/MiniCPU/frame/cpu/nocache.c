/* Hardware: the stand-in for the cache of lab 2, a machine without a
 * cache. Every access goes straight to physical memory: a memcpy at
 * dram_ptr, the file dram.img mapped into this process, which the caches
 * of the host serve as they serve any memory. An access lies within one
 * line, as for the cache of lab 2. It counts neither as a hit nor as a
 * miss.
 *
 * From lab 2 on, mini-cpu --cache 0 runs the machine without a cache in
 * another way: through uncached_read and uncached_write of
 * frame/cpu/dram.c, every access is one transfer, which removes the line
 * from the caches of the host and so reads or writes the DRAM of the host.
 */
#include <string.h>

#include "mcpu.h"

void cache_init(unsigned sets, unsigned ways)
{
    (void)sets;
    (void)ways;
}

void cache_read(uint64_t pa, void *dst, unsigned nbytes)
{
    uint8_t *dram = dram_ptr(pa, nbytes);
    if (dram)
        memcpy(dst, dram, nbytes);
    else
        memset(dst, 0, nbytes);
}

void cache_write(uint64_t pa, const void *src, unsigned nbytes)
{
    uint8_t *dram = dram_ptr(pa, nbytes);
    if (dram)
        memcpy(dram, src, nbytes);
}

void cache_flush(void)
{
}
