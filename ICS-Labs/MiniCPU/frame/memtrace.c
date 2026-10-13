/* mini-cpu --memtrace FILE: every translation and every cache access, one
 * line each, for tools/cachemodel.py and the tests of labs 4 and 5.
 *
 *     t VA          mmu_translate
 *     r PA N        cache_read of N bytes
 *     w PA N        cache_write of N bytes
 *     f             cache_flush
 *
 * Addresses are hexadecimal. The Makefile links with
 * -Wl,--wrap=mmu_translate,--wrap=cache_read,... so that calls to these
 * functions from any other file, the framework and the labs alike, arrive
 * here first. The trace starts after power-on, when the counters of
 * --stats start, and ends after the last cache_flush.
 */
#include "mcpu.h"

FILE *memtrace_out;

int  __real_mmu_translate(uint64_t va, int acc, uint64_t *pa);
void __real_cache_read(uint64_t pa, void *dst, unsigned n);
void __real_cache_write(uint64_t pa, const void *src, unsigned n);
void __real_cache_flush(void);

int __wrap_mmu_translate(uint64_t va, int acc, uint64_t *pa)
{
    if (memtrace_out)
        fprintf(memtrace_out, "t %lx\n", (unsigned long)va);
    return __real_mmu_translate(va, acc, pa);
}

void __wrap_cache_read(uint64_t pa, void *dst, unsigned n)
{
    if (memtrace_out)
        fprintf(memtrace_out, "r %lx %u\n", (unsigned long)pa, n);
    __real_cache_read(pa, dst, n);
}

void __wrap_cache_write(uint64_t pa, const void *src, unsigned n)
{
    if (memtrace_out)
        fprintf(memtrace_out, "w %lx %u\n", (unsigned long)pa, n);
    __real_cache_write(pa, src, n);
}

void __wrap_cache_flush(void)
{
    if (memtrace_out)
        fputs("f\n", memtrace_out);
    __real_cache_flush();
}
