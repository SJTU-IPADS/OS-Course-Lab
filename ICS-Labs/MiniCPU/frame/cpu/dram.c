/* Hardware: the DRAM of the machine, guest physical memory, which is the
 * file dram.img mapped into this process.
 *
 * The contents stay on disk after the machine stops and can be inspected
 * with od(1). The machine reads and writes this memory a line at a time,
 * and every line moved goes to the DRAM of the host: dram_line_read
 * removes the line from the caches of the host (flush_line below) and then
 * copies it out of the mapping, so the copy reads the DRAM of the host;
 * dram_line_write copies the line into the mapping and then removes it
 * from those caches, which writes it to the DRAM of the host. Only the
 * cache (lab 2) calls these two functions. The cache keeps its lines in
 * arrays, which the caches of the host hold, so a hit costs a few loads
 * and stores while a line moved to or from DRAM costs an access to the
 * DRAM of the host. The time of a whole run, as time(1) measures it, shows
 * what the cache saves.
 *
 * Without a cache, mini-cpu --cache 0, every access goes to DRAM:
 * uncached_read and uncached_write below make one transfer per call, since
 * a call of cache_read or cache_write stays within one line.
 *
 * dram_ptr gives the same mapping, read and written without flush_line
 * and without the cache. It serves the stand-ins, the devices, the
 * software that clears a new frame (mmu_alloc_frame) or sets up memory
 * before the machine runs (the loader, mini-jit) and the software that
 * shares buffers with a device (the driver of the block device and its
 * file system). The stand-in of the cache before lab 2,
 * frame/cpu/nocache.c, reads and writes there, so before lab 2 the caches
 * of the host serve every access.
 *
 * Two machines using the same file would overwrite each other's memory,
 * so the file is locked for as long as the machine runs.
 *
 * A second range of physical addresses, from ROM_BASE, holds a file the
 * guest has mapped with mmap (frame/os/sysbase.c): the model of
 * guest/mini-ollama, which is larger than the memory above. Its lines are
 * read from the mapping of that file in the same way, and nothing is
 * written to it.
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__x86_64__)
#include <cpuid.h>
#endif

#include "mcpu.h"

uint64_t dram_reads, dram_writes;
uint64_t cache_hits, cache_misses, cache_writebacks;

static uint8_t *ram;
static size_t   ram_size;
static int      ram_fd = -1;
static uint8_t *rom;
static size_t   rom_size;               /* a whole number of pages */
static uint64_t rom_bytes;              /* the size of the file */

/* ------------------------------------------------- the host's caches */

/* flush_line(addr) writes the line of the host that holds addr to the DRAM
   of the host if it is dirty, removes it from every cache of the host, and
   waits until both are done, so the next load from addr reads the DRAM of
   the host. A line of the host is 64 bytes or longer, so one call covers
   one line of the guest. x86-64 does this with clflush followed by mfence,
   arm64 with dc civac followed by dsb ish, both allowed in a user process.
   On an x86-64 host whose cpuid does not report clflush, on other hosts,
   and when the environment variable MCPU_NOFLUSH is set, flush_line does
   nothing: the program gives the same results, and only a transfer takes
   less time (tests/lab2/run.sh compares the two). */
static int flush_on;

static void flush_init(void)
{
#if defined(__x86_64__)
    unsigned eax, ebx, ecx, edx;          /* leaf 1: edx bit 19 is clflush */
    flush_on = __get_cpuid(1, &eax, &ebx, &ecx, &edx) && (edx >> 19 & 1);
#elif defined(__aarch64__)
    flush_on = 1;
#endif
    if (getenv("MCPU_NOFLUSH"))
        flush_on = 0;
}

static void flush_line(const void *addr)
{
    if (!flush_on)
        return;
#if defined(__x86_64__)
    __asm__ volatile("clflush (%0)\n\tmfence" : : "r"(addr) : "memory");
#elif defined(__aarch64__)
    __asm__ volatile("dc civac, %0\n\tdsb ish" : : "r"(addr) : "memory");
#else
    (void)addr;
#endif
}

int dram_open(const char *path, size_t size)
{
    ram_fd = open(path, O_RDWR | O_CREAT, 0644);
    if (ram_fd < 0) {
        perror(path);
        return -1;
    }
    if (flock(ram_fd, LOCK_EX | LOCK_NB) < 0) {
        fprintf(stderr, "%s is in use by another machine; pick another file with --ram\n", path);
        close(ram_fd);
        ram_fd = -1;
        return -1;
    }
    if (ftruncate(ram_fd, (off_t)size) < 0) {
        perror("ftruncate");
        return -1;
    }
    ram = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, ram_fd, 0);
    if (ram == MAP_FAILED) {
        perror("mmap");
        ram = NULL;
        return -1;
    }
    ram_size = size;
    flush_init();
    return 0;
}

void dram_close(void)
{
    if (ram)
        munmap(ram, ram_size);
    if (ram_fd >= 0)
        close(ram_fd);
    ram = NULL;
    ram_fd = -1;
}

uint64_t dram_rom(int fd)
{
    struct stat sb;
    if (fstat(fd, &sb) < 0 || !S_ISREG(sb.st_mode) || sb.st_size <= 0)
        return 0;
    if (!rom) {
        size_t size = ((size_t)sb.st_size + PAGE_SIZE - 1) & ~(size_t)(PAGE_SIZE - 1);
        /* Private and writable: the guest maps these pages read-only, and
           if a write arrived all the same it would not reach the file. */
        void *mapping = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (mapping == MAP_FAILED)
            return 0;
        rom = mapping;
        rom_size = size;
        rom_bytes = (uint64_t)sb.st_size;
    }
    return (uint64_t)sb.st_size;
}

uint8_t *dram_ptr(uint64_t pa, size_t nbytes)
{
    if (pa >= ROM_BASE) {
        if (!rom || pa - ROM_BASE + nbytes > rom_size || pa + nbytes < pa)
            return NULL;
        return rom + (pa - ROM_BASE);
    }
    if (!ram || pa + nbytes > ram_size || pa + nbytes < pa)
        return NULL;
    return ram + pa;
}

/* ------------------------------------------------------- transfers */

/* One transfer: nbytes bytes at pa, all inside one line, copied out of the
   mapping after the line has left the caches of the host. Bytes outside
   memory read as zeros. */
static void transfer_read(uint64_t pa, uint8_t *dst, unsigned nbytes)
{
    uint8_t *base = ram;
    uint64_t offset = pa, limit = ram_size;
    if (pa >= ROM_BASE) {
        base = rom;
        offset = pa - ROM_BASE;
        limit = rom_bytes;
    }
    memset(dst, 0, nbytes);
    if (base && offset < limit) {
        uint64_t avail = limit - offset;
        flush_line(base + offset);
        memcpy(dst, base + offset, avail < nbytes ? avail : nbytes);
    }
    ++dram_reads;
}

/* One transfer: nbytes bytes at pa, all inside one line, copied into the
   mapping, after which the line leaves the caches of the host. Only the
   bytes given change; nothing outside memory is written, and nothing in
   the file at ROM_BASE. */
static void transfer_write(uint64_t pa, const uint8_t *src, unsigned nbytes)
{
    if (ram && pa < ram_size && pa + nbytes <= ram_size) {
        memcpy(ram + pa, src, nbytes);
        flush_line(ram + pa);
    }
    ++dram_writes;
}

void dram_line_read(uint64_t pa, uint8_t *dst)
{
    transfer_read(pa & ~(uint64_t)(LINE_SIZE - 1), dst, LINE_SIZE);
}

void dram_line_write(uint64_t pa, const uint8_t *src)
{
    transfer_write(pa & ~(uint64_t)(LINE_SIZE - 1), src, LINE_SIZE);
}

/* ------------------------------------------------- without a cache */

/* An access of nbytes bytes at pa, all inside one line (frame/memtrace.c
   checks this), is one transfer. */
void uncached_read(uint64_t pa, void *dst, unsigned nbytes)
{
    transfer_read(pa, dst, nbytes);
}

/* A write to part of a line is one transfer as well, and changes only the
   bytes written, as a write burst of DDR memory with its byte mask does. */
void uncached_write(uint64_t pa, const void *src, unsigned nbytes)
{
    transfer_write(pa, src, nbytes);
}

/* -------------------------------------------------------- statistics */

void memory_stats(FILE *out)
{
    if (opt_nocache) {
        fputs("cache off\n", out);
    } else {
        uint64_t accesses = cache_hits + cache_misses;
        fprintf(out, "cache accesses %lu  hits %lu  misses %lu  hit rate %.2f%%  writebacks %lu\n",
                (unsigned long)accesses, (unsigned long)cache_hits,
                (unsigned long)cache_misses,
                accesses ? 100.0 * (double)cache_hits / (double)accesses : 0.0,
                (unsigned long)cache_writebacks);
    }
    fprintf(out, "dram  line reads %lu  line writes %lu\n",
            (unsigned long)dram_reads, (unsigned long)dram_writes);
}
