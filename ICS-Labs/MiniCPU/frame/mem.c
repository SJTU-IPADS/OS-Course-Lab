/* Virtual memory as the CPU sees it: translate, then go through the cache.
 *
 * An access may cross a page boundary, and two neighbouring virtual pages
 * may lie far apart in physical memory, so each page is translated alone.
 */
#include "mcpu.h"

static int walk(mcpu *c, uint64_t va, uint8_t *p, unsigned n, int acc)
{
    while (n) {
        unsigned off = (unsigned)(va % PAGE_SIZE);
        unsigned k = PAGE_SIZE - off < n ? PAGE_SIZE - off : n;
        uint64_t pa;
        if (mmu_translate(va, acc, &pa) < 0) {
            c->fault_va = va;
            return -1;
        }
        if (acc == MEM_WRITE)
            cache_write(pa, p, k);
        else
            cache_read(pa, p, k);
        va += k; p += k; n -= k;
    }
    return 0;
}

int mem_read(mcpu *c, uint64_t va, void *dst, unsigned n, int acc)
{
    return walk(c, va, dst, n, acc);
}

int mem_write(mcpu *c, uint64_t va, const void *src, unsigned n)
{
    return walk(c, va, (uint8_t *)(uintptr_t)src, n, MEM_WRITE);
}

int guest_write(mcpu *c, uint64_t va, const void *src, uint64_t n)
{
    static uint8_t zero[256];
    if (src)
        return walk(c, va, (uint8_t *)(uintptr_t)src, (unsigned)n, MEM_WRITE);
    for (uint64_t off = 0; off < n; off += sizeof zero) {   /* n zero bytes */
        uint64_t k = n - off < sizeof zero ? n - off : sizeof zero;
        if (walk(c, va + off, zero, (unsigned)k, MEM_WRITE) < 0)
            return -1;
    }
    return 0;
}

/* Byte by byte up to the first byte that cannot be translated, so that an
   instruction ending just before an unmapped page still decodes. A fetch
   may be speculative in lab 2, hence no fault is recorded here. */
unsigned fetch_bytes(mcpu *c, uint64_t pc, uint8_t *buf)
{
    unsigned n = 0;
    uint64_t save = c->fault_va;
    while (n < MAX_INSN) {
        uint64_t va = pc + n;
        unsigned k = PAGE_SIZE - (unsigned)(va % PAGE_SIZE);
        if (k > MAX_INSN - n)
            k = MAX_INSN - n;
        if (walk(c, va, buf + n, k, MEM_FETCH) < 0)
            break;
        n += k;
    }
    c->fault_va = save;
    return n;
}
