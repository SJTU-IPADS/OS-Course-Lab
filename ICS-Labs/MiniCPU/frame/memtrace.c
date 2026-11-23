/* Neither hardware nor software: a probe on the wires between the CPU,
 * the MMU and the cache, for the tests and for mini-cpu --cache 0.
 *
 * The Makefile links with -Wl,--wrap=mmu_translate,--wrap=cache_read,...
 * so that calls to these functions from any other file, the framework and
 * the labs alike, arrive here first. This file does three things with them.
 *
 * mini-cpu --memtrace FILE writes one line per call, for
 * tools/cachemodel.py and the tests of labs 2 and 3:
 *
 *     t VA          mmu_translate
 *     r PA N        cache_read of N bytes
 *     w PA N        cache_write of N bytes
 *     f             cache_flush
 *
 * Addresses are hexadecimal. The trace starts after power-on, when the
 * counters of --stats start, and ends after the last cache_flush.
 *
 * Every cache_read and cache_write stays within one line: (pa % LINE_SIZE)
 * + nbytes <= LINE_SIZE, with nbytes at least 1. The memory access unit of
 * the CPU (frame/cpu/mem.c) splits its accesses so. A call that breaks
 * this rule is an error of the framework: it is reported on stderr and
 * the program ends with status 4.
 *
 * mini-cpu --cache 0 sets opt_nocache: the machine has no cache, and
 * cache_read and cache_write go to uncached_read and uncached_write of
 * frame/cpu/dram.c instead of the cache of lab 2; cache_flush does nothing.
 * The trace is the same with and without the cache.
 */
#include "mcpu.h"

FILE *memtrace_out;
int   opt_nocache;

int  __real_mmu_translate(uint64_t va, int acc, uint64_t *pa);
void __real_cache_read(uint64_t pa, void *dst, unsigned nbytes);
void __real_cache_write(uint64_t pa, const void *src, unsigned nbytes);
void __real_cache_flush(void);

/* The rule of the cache: one call, one line. */
static void one_line(const char *who, uint64_t pa, unsigned nbytes)
{
    if (nbytes >= 1 && pa % LINE_SIZE + nbytes <= LINE_SIZE)
        return;
    fprintf(stderr, "framework error: %s of %u bytes at physical address %#lx"
                    " does not lie within one %u-byte line\n",
            who, nbytes, (unsigned long)pa, LINE_SIZE);
    exit(4);
}

int __wrap_mmu_translate(uint64_t va, int acc, uint64_t *pa)
{
    if (memtrace_out)
        fprintf(memtrace_out, "t %lx\n", (unsigned long)va);
    return __real_mmu_translate(va, acc, pa);
}

void __wrap_cache_read(uint64_t pa, void *dst, unsigned nbytes)
{
    if (memtrace_out)
        fprintf(memtrace_out, "r %lx %u\n", (unsigned long)pa, nbytes);
    one_line("cache_read", pa, nbytes);
    if (opt_nocache)
        uncached_read(pa, dst, nbytes);
    else
        __real_cache_read(pa, dst, nbytes);
}

void __wrap_cache_write(uint64_t pa, const void *src, unsigned nbytes)
{
    if (memtrace_out)
        fprintf(memtrace_out, "w %lx %u\n", (unsigned long)pa, nbytes);
    one_line("cache_write", pa, nbytes);
    if (opt_nocache)
        uncached_write(pa, src, nbytes);
    else
        __real_cache_write(pa, src, nbytes);
}

void __wrap_cache_flush(void)
{
    if (memtrace_out)
        fputs("f\n", memtrace_out);
    if (!opt_nocache)
        __real_cache_flush();
}
