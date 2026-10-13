/* Stand-in for lab 5: no cache. Every access goes straight to physical
 * memory and counts neither as a hit nor as a miss, so the pipeline of
 * lab 2 spends one cycle in MEM for every access.
 */
#include <string.h>

#include "mcpu.h"

void cache_init(unsigned sets, unsigned ways)
{
    (void)sets;
    (void)ways;
}

void cache_read(uint64_t pa, void *dst, unsigned n)
{
    uint8_t *p = dram_ptr(pa, n);
    if (p)
        memcpy(dst, p, n);
    else
        memset(dst, 0, n);
}

void cache_write(uint64_t pa, const void *src, unsigned n)
{
    uint8_t *p = dram_ptr(pa, n);
    if (p)
        memcpy(p, src, n);
}

void cache_flush(void)
{
}
