/* Software: the loader, which loads a statically linked, non-PIE ET_EXEC
 * into the guest.
 *
 * The loader runs without a kernel, so the page tables are filled in one
 * page at a time:
 *
 *   1. every PT_LOAD segment gets physical frames mapped at its address;
 *   2. the part of a segment beyond p_filesz (.bss) is cleared;
 *   3. argc / argv / envp / auxv are laid out at the top of the stack as
 *      the System V ABI specifies;
 *   4. rip is set to e_entry and rsp to the top of the stack.
 */
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mcpu.h"

#define STACK_TOP   0x00007ffffffff000ULL
#define STACK_PAGES 64

static uint64_t page_down(uint64_t addr) { return addr & ~(uint64_t)(PAGE_SIZE - 1); }
static uint64_t page_up(uint64_t addr)   { return page_down(addr + PAGE_SIZE - 1); }

/* Make sure [va, va+nbytes) is backed by frames; mapped pages are kept. */
static int ensure(uint64_t va, uint64_t nbytes, uint64_t flags)
{
    for (uint64_t page = page_down(va); page < page_up(va + nbytes); page += PAGE_SIZE) {
        uint64_t pa;
        if (mmu_translate(page, MEM_READ, &pa) == 0)
            continue;
        uint64_t frame = mmu_alloc_frame();
        if (!frame || mmu_map(page, frame, flags) < 0) {
            fprintf(stderr, "load: mapping virtual page %#lx failed\n",
                    (unsigned long)page);
            return -1;
        }
    }
    mmu_tlb_flush();
    return 0;
}

/* Write bytes at a guest virtual address, zeros when src is NULL. The
   machine is not running yet, so this bypasses the cache. */
static int put(uint64_t va, const void *src, uint64_t nbytes)
{
    const uint8_t *from = src;
    while (nbytes) {
        uint64_t pa;
        if (mmu_translate(va, MEM_READ, &pa) < 0) {
            fprintf(stderr, "load: virtual address %#lx was just mapped but does not translate;"
                            " the page table walk cannot find the mapping\n", (unsigned long)va);
            return -1;
        }
        uint64_t chunk = PAGE_SIZE - va % PAGE_SIZE;
        if (chunk > nbytes) chunk = nbytes;
        uint8_t *dst = dram_ptr(pa, chunk);
        if (!dst) {
            fprintf(stderr, "load: physical address %#lx is out of range\n", (unsigned long)pa);
            return -1;
        }
        if (from) memcpy(dst, from, chunk); else memset(dst, 0, chunk);
        va += chunk; nbytes -= chunk; if (from) from += chunk;
    }
    return 0;
}

int loader_exec(mcpu *cpu, const char *path, char **argv, int argc)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror(path); return -1; }
    struct stat sb;
    if (fstat(fd, &sb) < 0) { perror("fstat"); close(fd); return -1; }
    uint8_t *img = mmap(NULL, (size_t)sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (img == MAP_FAILED) { perror("mmap"); return -1; }

    Elf64_Ehdr *eh = (Elf64_Ehdr *)img;
    if ((size_t)sb.st_size < sizeof *eh || memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0 ||
        eh->e_ident[EI_CLASS] != ELFCLASS64 || eh->e_machine != EM_X86_64) {
        fprintf(stderr, "%s: not an x86-64 ELF file\n", path);
        return -1;
    }
    if (eh->e_type != ET_EXEC) {
        fprintf(stderr, "%s: only statically linked, non-PIE executables are supported"
                        " (e_type=%u)\n", path, eh->e_type);
        return -1;
    }

    /* root of the page tables */
    uint64_t pml4 = 0x1000;
    memset(dram_ptr(pml4, PAGE_SIZE), 0, PAGE_SIZE);
    mmu_set_cr3(pml4);

    uint64_t top = 0;
    Elf64_Phdr *ph = (Elf64_Phdr *)(img + eh->e_phoff);
    for (unsigned idx = 0; idx < eh->e_phnum; ++idx) {
        if (ph[idx].p_type != PT_LOAD)
            continue;
        uint64_t flags = PTE_P | PTE_U;
        if (ph[idx].p_flags & PF_W) flags |= PTE_W;
        if (ensure(ph[idx].p_vaddr, ph[idx].p_memsz, flags) < 0)
            return -1;
        if (put(ph[idx].p_vaddr, img + ph[idx].p_offset, ph[idx].p_filesz) < 0)
            return -1;
        if (ph[idx].p_memsz > ph[idx].p_filesz &&
            put(ph[idx].p_vaddr + ph[idx].p_filesz, NULL,
                ph[idx].p_memsz - ph[idx].p_filesz) < 0)
            return -1;
        if (ph[idx].p_vaddr + ph[idx].p_memsz > top)
            top = ph[idx].p_vaddr + ph[idx].p_memsz;
    }
    uint64_t entry = eh->e_entry;
    munmap(img, (size_t)sb.st_size);
    cpu->brk = page_up(top);

    /* stack */
    uint64_t stack_lo = STACK_TOP - (uint64_t)STACK_PAGES * PAGE_SIZE;
    if (ensure(stack_lo, (uint64_t)STACK_PAGES * PAGE_SIZE,
               PTE_P | PTE_W | PTE_U) < 0)
        return -1;

    /* argument strings first, right below the top, then the pointers */
    uint64_t sp = STACK_TOP;
    uint64_t ptr[16];
    if (argc > 15) argc = 15;
    for (int arg = argc - 1; arg >= 0; --arg) {
        uint64_t nbytes = strlen(argv[arg]) + 1;
        sp -= nbytes;
        put(sp, argv[arg], nbytes);
        ptr[arg] = sp;
    }
    sp &= ~15ULL;
    /* auxv terminator, envp terminator, argv terminator, argv, argc */
    uint64_t zero = 0;
    sp -= 8; put(sp, &zero, 8);                 /* AT_NULL value */
    sp -= 8; put(sp, &zero, 8);                 /* AT_NULL */
    sp -= 8; put(sp, &zero, 8);                 /* envp[0] = NULL */
    sp -= 8; put(sp, &zero, 8);                 /* argv[argc] = NULL */
    for (int arg = argc - 1; arg >= 0; --arg) { sp -= 8; put(sp, &ptr[arg], 8); }
    uint64_t argc_word = (uint64_t)argc;
    sp -= 8; put(sp, &argc_word, 8);

    memset(cpu->r, 0, sizeof cpu->r);
    cpu->r[RSP] = sp;
    cpu->rip = entry;
    cpu->rflags = 0x2;
    mmu_tlb_flush();
    return 0;
}
