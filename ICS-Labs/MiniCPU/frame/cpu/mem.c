/* Hardware: the memory access unit of the CPU.
 *
 * An instruction reaches memory at a virtual address: the fetch of its
 * bytes (fetch_bytes, called by x86_begin), a load or a store of 1 to 8
 * bytes (load and store of frame/cpu/x86.c), a vector operand of up to 32
 * bytes (rm_vget and rm_vset). One access is at most LINE_SIZE bytes. A
 * longer one is an error of the framework: it is reported on stderr and
 * the program ends with status 4. The address may be any byte, so one such
 * access may cross the boundary of a cache line and even of a page, and
 * it lies in at most two lines and two pages. This unit splits it as the
 * hardware does:
 *
 *   1. page by page: each page is translated alone with mmu_translate,
 *      since two neighbouring virtual pages may lie far apart in physical
 *      memory;
 *   2. within a page, line by line: each line is one cache access,
 *      cache_read or cache_write, which never crosses a line.
 *
 *   load of 8 bytes at line offset 60:   t va   r pa 4   r pa+4 4
 *
 * A page that does not translate ends the access: cpu->fault_va becomes
 * the first address of that page (va itself for the first page), and the
 * pages before it have already been read or written.
 *
 * Software that copies a range of any length (the system calls, the host
 * functions, the object loader of lab 4 and mini-jit) does not call this
 * unit with the whole range: guest_read and guest_write of frame/os/copy.c
 * cut the range at line boundaries and make one access per piece, as the
 * loop of memcpy does. The loader of the program, frame/os/loader.c, fills
 * guest memory before the machine starts, with dram_ptr and without this
 * unit.
 */
#include "mcpu.h"

/* nbytes bytes at va, for an access of kind acc: translate each page, then
   one cache access per line of the page. */
static int mem_access(mcpu *cpu, uint64_t va, uint8_t *buf, unsigned nbytes, int acc)
{
    if (nbytes > LINE_SIZE) {
        fprintf(stderr, "framework error: an access of %u bytes at virtual address %#lx"
                        " is longer than one %u-byte line\n",
                nbytes, (unsigned long)va, LINE_SIZE);
        exit(4);
    }
    while (nbytes) {
        unsigned in_page = PAGE_SIZE - (unsigned)(va % PAGE_SIZE);
        unsigned chunk = in_page < nbytes ? in_page : nbytes;
        uint64_t pa;
        if (mmu_translate(va, acc, &pa) < 0) {
            cpu->fault_va = va;
            return -1;
        }
        for (unsigned done = 0; done < chunk;) {
            unsigned in_line = LINE_SIZE - (unsigned)((pa + done) % LINE_SIZE);
            unsigned count = in_line < chunk - done ? in_line : chunk - done;
            if (acc == MEM_WRITE)
                cache_write(pa + done, buf + done, count);
            else
                cache_read(pa + done, buf + done, count);
            done += count;
        }
        va += chunk; buf += chunk; nbytes -= chunk;
    }
    return 0;
}

int mem_read(mcpu *cpu, uint64_t va, void *dst, unsigned nbytes, int acc)
{
    return mem_access(cpu, va, dst, nbytes, acc);
}

int mem_write(mcpu *cpu, uint64_t va, const void *src, unsigned nbytes)
{
    return mem_access(cpu, va, (uint8_t *)(uintptr_t)src, nbytes, MEM_WRITE);
}

/* Page by page up to the first byte that cannot be translated, so that an
   instruction ending just before an unmapped page still decodes. The
   instruction may end before that byte, hence no fault is recorded here:
   the interpreter reports one when it reads a byte that is missing. */
unsigned fetch_bytes(mcpu *cpu, uint64_t pc, uint8_t *bytes)
{
    unsigned nbytes = 0;
    uint64_t save = cpu->fault_va;
    while (nbytes < MAX_INSN) {
        uint64_t va = pc + nbytes;
        unsigned chunk = PAGE_SIZE - (unsigned)(va % PAGE_SIZE);
        if (chunk > MAX_INSN - nbytes)
            chunk = MAX_INSN - nbytes;
        if (mem_access(cpu, va, bytes + nbytes, chunk, MEM_FETCH) < 0)
            break;
        nbytes += chunk;
    }
    cpu->fault_va = save;
    return nbytes;
}
