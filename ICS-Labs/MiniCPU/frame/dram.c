/* Guest physical memory: a file mapped with mmap.
 *
 * The contents stay on disk after the machine stops and can be inspected
 * with od(1). Apart from the stand-ins and the loader, which run while the
 * machine is being set up, only the cache (lab 5) reads and writes here,
 * and it does so a whole line at a time.
 *
 * Two machines mapping the same file would overwrite each other's memory,
 * so the file is locked for as long as the machine runs.
 *
 * A second range of physical addresses, from ROM_BASE, holds a file the
 * guest has mapped with mmap (frame/sysbase.c): the model of
 * guest/mini-ollama, which is larger than the memory above. The host maps
 * that file as well, so nothing is copied and only the pages the guest
 * touches are read from disk.
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mcpu.h"

uint64_t dram_reads, dram_writes;
uint64_t cache_hits, cache_misses, cache_writebacks;

static uint8_t *ram;
static size_t   ram_size;
static int      ram_fd = -1;
static uint8_t *rom;
static size_t   rom_size;               /* a whole number of pages */

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
        void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (p == MAP_FAILED)
            return 0;
        rom = p;
        rom_size = size;
    }
    return (uint64_t)sb.st_size;
}

uint8_t *dram_ptr(uint64_t pa, size_t n)
{
    if (pa >= ROM_BASE) {
        if (!rom || pa - ROM_BASE + n > rom_size || pa + n < pa)
            return NULL;
        return rom + (pa - ROM_BASE);
    }
    if (!ram || pa + n > ram_size || pa + n < pa)
        return NULL;
    return ram + pa;
}

void dram_line_read(uint64_t pa, uint8_t *dst)
{
    uint8_t *p = dram_ptr(pa & ~(uint64_t)(LINE_SIZE - 1), LINE_SIZE);
    if (!p) {
        memset(dst, 0, LINE_SIZE);
        return;
    }
    memcpy(dst, p, LINE_SIZE);
    ++dram_reads;
}

void dram_line_write(uint64_t pa, const uint8_t *src)
{
    uint8_t *p = dram_ptr(pa & ~(uint64_t)(LINE_SIZE - 1), LINE_SIZE);
    if (!p)
        return;
    memcpy(p, src, LINE_SIZE);
    ++dram_writes;
}
