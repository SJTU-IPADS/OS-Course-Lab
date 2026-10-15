/* The system calls of the guest.
 *
 * Numbers and arguments follow the Linux x86-64 convention, so the guest
 * makes ordinary system calls:
 *
 *     number in rax, arguments in rdi rsi rdx r10 r8 r9, result in rax
 *
 * write goes to the host's stdout or stderr, exit stops the machine, brk
 * maps the heap page by page, getpid returns 1. read, open and close are
 * the three functions of src/lab6/syscall.c; before lab 6 they return
 * -ENOSYS, as every other number does.
 *
 * fstat and mmap exist for one use: a program maps the file on its
 * standard input,
 *
 *     fstat(0, &st);
 *     p = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, 0, 0);
 *
 * and reads it as memory. guest/mini-ollama gets its model this way
 * (mini-cpu guest/mini-ollama PROMPT < MODEL.gguf): the file is a gigabyte,
 * the memory of the machine 64 MiB. The pages of the file are read-only
 * memory at ROM_BASE (frame/dram.c), mapped here into the address space of
 * the guest. No other form of mmap is implemented.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mcpu.h"

uint64_t syscall_count;

enum { SYS_read = 0, SYS_write = 1, SYS_open = 2, SYS_close = 3, SYS_fstat = 5,
       SYS_mmap = 9, SYS_brk = 12, SYS_getpid = 39, SYS_exit = 60,
       SYS_exit_group = 231 };

#define MMAP_BASE 0x00007f0000000000ULL /* mappings go upwards from here */

static long sys_brk(mcpu *c, uint64_t want)
{
    if (want == 0 || want < c->brk)
        return (long)c->brk;
    uint64_t p = c->brk;
    while (p < want) {
        uint64_t f = mmu_alloc_frame();
        if (!f || mmu_map(p, f, PTE_P | PTE_W | PTE_U) < 0)
            return (long)c->brk;
        p += PAGE_SIZE;
    }
    mmu_tlb_flush();
    c->brk = p;
    return (long)c->brk;
}

/* fstat(fd, buf) of descriptor 0, 1 or 2 of the host: struct stat of Linux
   x86-64, 144 bytes, with st_mode (offset 24) and st_size (offset 48); the
   other fields are zero. */
static long sys_fstat(mcpu *c, uint64_t fd, uint64_t buf)
{
    struct stat sb;
    uint8_t out[144] = { 0 };

    if (fd > 2)
        return -EBADF;
    if (fstat((int)fd, &sb) < 0)
        return -errno;
    uint32_t mode = (uint32_t)sb.st_mode;
    uint64_t size = (uint64_t)sb.st_size;
    memcpy(out + 24, &mode, sizeof mode);
    memcpy(out + 48, &size, sizeof size);
    return mem_write(c, buf, out, sizeof out) < 0 ? -EFAULT : 0;
}

/* mmap(NULL, len, PROT_READ, MAP_PRIVATE, 0, 0): the file on standard
   input, read-only, at an address chosen here. */
static long sys_mmap(uint64_t addr, uint64_t len, uint64_t prot, uint64_t flags,
                     uint64_t fd, uint64_t off)
{
    static uint64_t next = MMAP_BASE;

    if (addr != 0 || prot != 1 || flags != 2 || fd != 0 || off != 0) {
        fprintf(stderr, "mmap: only mmap(NULL, len, PROT_READ, MAP_PRIVATE, 0, 0),"
                        " a read-only mapping of standard input, is implemented\n");
        return -EINVAL;
    }
    uint64_t size = dram_rom(0);
    if (len == 0 || size == 0)
        return -EINVAL;                                 /* standard input is not a file */
    if (len > size)
        len = size;
    uint64_t va = next;
    for (uint64_t o = 0; o < len; o += PAGE_SIZE)
        if (mmu_map(va + o, ROM_BASE + o, PTE_P | PTE_U) < 0)
            return -ENOMEM;
    mmu_tlb_flush();
    next += (len + PAGE_SIZE - 1) & ~(uint64_t)(PAGE_SIZE - 1);
    return (long)va;
}

int sys_dispatch(mcpu *c)
{
    uint64_t nr = c->r[RAX], a0 = c->r[RDI], a1 = c->r[RSI], a2 = c->r[RDX];
    long ret = -ENOSYS;
    ++syscall_count;

    switch (nr) {
    case SYS_write: {
        char buf[4096];
        unsigned n = a2 > sizeof buf ? sizeof buf : (unsigned)a2;
        if (mem_read(c, a1, buf, n, MEM_READ) < 0) {
            ret = -EFAULT;
            break;
        }
        ret = write(a0 == 2 ? 2 : 1, buf, n);
        break;
    }
    case SYS_brk:
        ret = sys_brk(c, a0);
        break;
    case SYS_fstat:
        ret = sys_fstat(c, a0, a1);
        break;
    case SYS_mmap:
        ret = sys_mmap(a0, a1, a2, c->r[R10], c->r[R8], c->r[R9]);
        break;
    case SYS_getpid:
        ret = 1;
        break;
    case SYS_exit:
    case SYS_exit_group:
        c->halted = 1;
        ret = 0;
        break;
#if LAB >= 6
    case SYS_read:
        ret = sys_read(c, a0, a1, a2);
        break;
    case SYS_open:
        ret = sys_open(c, a0);
        break;
    case SYS_close:
        ret = sys_close(c, a0);
        break;
    default:
        fprintf(stderr, "unimplemented system call %lu, rip=%#lx\n",
                (unsigned long)nr, (unsigned long)c->rip);
        break;
#else
    default:
        fprintf(stderr, "system call %lu is implemented in lab 6, rip=%#lx\n",
                (unsigned long)nr, (unsigned long)c->rip);
        break;
#endif
    }
    c->r[RAX] = (uint64_t)ret;
    return 0;
}
