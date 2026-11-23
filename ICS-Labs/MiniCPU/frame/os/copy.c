/* Software: copying a range of guest memory, as copy_from_user and
 * copy_to_user do in a kernel.
 *
 * The system calls (frame/os/sysbase.c, src/lab5), the host functions of
 * frame/os/hostcall.c, the loader of lab 4 and mini-jit move buffers of
 * any length between the host and the virtual memory of the guest. They
 * do it as the loop of memcpy does: the range is cut at the boundaries of
 * the cache lines, and each piece is one access of the CPU, mem_read or
 * mem_write of frame/cpu/mem.c. A piece lies within one line, and so
 * within one page: it is one translation and one cache access.
 *
 *   guest_write of 100 bytes at line offset 40:
 *       mem_write 24 bytes, mem_write 64 bytes, mem_write 12 bytes
 *
 * A piece whose page does not translate ends the copy: the function
 * returns -1 with cpu->fault_va set to the first address of that piece,
 * and the pieces before it have been copied.
 */
#include "mcpu.h"

/* The bytes from va to the end of its line, at most nbytes. */
static unsigned line_piece(uint64_t va, uint64_t nbytes)
{
    unsigned rest = LINE_SIZE - (unsigned)(va % LINE_SIZE);
    return nbytes < rest ? (unsigned)nbytes : rest;
}

int guest_read(mcpu *cpu, uint64_t va, void *dst, uint64_t nbytes)
{
    uint8_t *into = dst;
    while (nbytes) {
        unsigned count = line_piece(va, nbytes);
        if (mem_read(cpu, va, into, count, MEM_READ) < 0)
            return -1;
        va += count; into += count; nbytes -= count;
    }
    return 0;
}

int guest_write(mcpu *cpu, uint64_t va, const void *src, uint64_t nbytes)
{
    static const uint8_t zero[LINE_SIZE];
    const uint8_t *from = src;
    while (nbytes) {
        unsigned count = line_piece(va, nbytes);
        if (mem_write(cpu, va, from ? from : zero, count) < 0)
            return -1;
        va += count; nbytes -= count;
        if (from)
            from += count;
    }
    return 0;
}
