/* Neither hardware nor software of the machine: the driver, a program of
 * the host. It powers the machine on, loads a program, runs it on the
 * interpreter of lab 1 and prints statistics.
 *
 *   mini-cpu [options] program [args...]
 *
 *     --ram FILE        file used as physical memory (default dram.img)
 *     --max N           stop after N instructions
 *     --commit FILE     commit log, - for stdout
 *     -v                one line per instruction on stderr
 *     --stats           statistics on stderr at the end
 *     --memtrace FILE   translations and cache accesses (frame/memtrace.c)
 *     --disasm          print the code of program as assembly and exit
 *   lab 2
 *     --cache S,W       sets and ways of the cache (default 64,4); S = 0:
 *                       no cache, every cache access is one DRAM
 *                       transfer (frame/cpu/dram.c)
 *   lab 5
 *     --disk FILE       block device image (default disk.img, if present)
 *     --ls              list the files in the image and exit
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcpu.h"

#define RING_PA 0x800

static void usage(void)
{
    fputs("usage: mini-cpu [--ram F] [--max N] [--commit F] [-v] [--stats] [--memtrace F] [--disasm]"
#if LAB >= 2
          " [--cache S,W]"
#endif
#if LAB >= 5
          " [--disk F] [--ls]"
#endif
          " program [args...]\n", stderr);
}

#if LAB >= 2
/* The cache of lab 2 with sets * ways lines, or none when sets is 0. */
static void start_cache(unsigned sets, unsigned ways)
{
    opt_nocache = sets == 0;
    if (!opt_nocache)
        cache_init(sets, ways);
}
#endif

static FILE *open_out(const char *path)
{
    if (!path)
        return NULL;
    if (!strcmp(path, "-"))
        return stdout;
    FILE *out = fopen(path, "w");
    if (!out)
        perror(path);
    return out;
}

int main(int argc, char **argv)
{
    const char *ram = "dram.img", *commit = NULL, *memtrace = NULL;
    int arg, stats = 0, dis = 0;
#if LAB >= 2
    unsigned sets = 64, ways = 4;
#endif
#if LAB >= 5
    const char *disk = "disk.img";
    int ls = 0;
#else
    const int ls = 0;
#endif

    for (arg = 1; arg < argc && argv[arg][0] == '-' && argv[arg][1]; ++arg) {
        const char *option = argv[arg], *value = arg + 1 < argc ? argv[arg + 1] : NULL;
        if (!strcmp(option, "--ram") && value)             ram = argv[++arg];
        else if (!strcmp(option, "--max") && value)        opt_max = strtoull(argv[++arg], NULL, 0);
        else if (!strcmp(option, "--commit") && value)     commit = argv[++arg];
        else if (!strcmp(option, "-v"))                    opt_steps = 1;
        else if (!strcmp(option, "--stats"))               stats = 1;
        else if (!strcmp(option, "--memtrace") && value)   memtrace = argv[++arg];
        else if (!strcmp(option, "--disasm"))              dis = 1;
#if LAB >= 2
        else if (!strcmp(option, "--cache") && value)      sscanf(argv[++arg], "%u,%u", &sets, &ways);
#endif
#if LAB >= 5
        else if (!strcmp(option, "--disk") && value)       disk = argv[++arg];
        else if (!strcmp(option, "--ls"))                  ls = 1;
#endif
        else { usage(); return 2; }
    }
    if (arg >= argc && !ls) { usage(); return 2; }
    if (dis)
        return disasm(argv[arg], stdout) < 0;

    if (dram_open(ram, GUEST_RAM) < 0)
        return 1;

#if LAB >= 2
    /* Page table reads and writes go through the cache, so it must exist
       before loading. */
    start_cache(sets, ways);
#else
    cache_init(0, 0);
#endif

#if LAB >= 5
    int has_disk = blk_attach(disk, RING_PA) == 0;
    if (ls) {
        if (!has_disk)
            return 1;
        mmu_set_cr3(0x1000);            /* ramfs only uses its bounce page */
        if (ramfs_mount() < 0)
            return 1;
        ramfs_list(stdout);
        dram_close();
        return 0;
    }
#endif

    mcpu cpu;
    memset(&cpu, 0, sizeof cpu);
    if (loader_exec(&cpu, argv[arg], argv + arg, argc - arg) < 0)
        return 1;
#if LAB >= 5
    if (has_disk && ramfs_mount() < 0)
        return 1;
#endif

    /* Loading and mounting are part of power-on and are not counted:
       write dirty lines back, then start over with an empty cache. */
    cache_flush();
#if LAB >= 2
    start_cache(sets, ways);
#endif
    mmu_tlb_flush();
    dram_reads = dram_writes = 0;
    tlb_hits = tlb_misses = page_faults = 0;
#if LAB >= 5
    blk_reads = blk_writes = 0;
#endif

    memtrace_out = open_out(memtrace);
    commit_out = open_out(commit);
    commit_start(&cpu);
    interp_run(&cpu);
    if (opt_max && cpu.icount >= opt_max && !cpu.halted && !cpu.fault)
        fprintf(stderr, "stopped after %lu instructions (--max)\n", (unsigned long)opt_max);

    /* The exit code of the guest becomes the exit code of mini-cpu. */
    int rc = cpu.halted ? (int)(cpu.r[RDI] & 0xff) : 0;
    if (cpu.fault) {
        fprintf(stderr, "\nfault (%s) after %lu instructions:\n",
                cpu.fault == FAULT_PAGE ? "page fault" :
                cpu.fault == FAULT_DIV ? "divide error" : "illegal instruction",
                (unsigned long)cpu.icount);
        if (cpu.fault == FAULT_PAGE)
            fprintf(stderr, "faulting address %#lx\n", (unsigned long)cpu.fault_va);
        cpu_dump(&cpu, stderr);
        rc = 1;
    }

    if (commit_out && commit_out != stdout)
        fclose(commit_out);
    cache_flush();
    if (memtrace_out && memtrace_out != stdout)
        fclose(memtrace_out);
    memtrace_out = NULL;

    if (stats) {
        fprintf(stderr, "insns %lu  syscalls %lu  exit %lu\n",
                (unsigned long)cpu.icount, (unsigned long)syscall_count,
                (unsigned long)(cpu.r[RDI] & 0xff));
#if LAB >= 3
        fprintf(stderr, "tlb   hits %lu  misses %lu  page faults %lu\n",
                (unsigned long)tlb_hits, (unsigned long)tlb_misses,
                (unsigned long)page_faults);
#endif
#if LAB >= 2
        memory_stats(stderr);
#endif
#if LAB >= 5
        fprintf(stderr, "disk  reads %lu  writes %lu\n",
                (unsigned long)blk_reads, (unsigned long)blk_writes);
#endif
    }
    dram_close();
    return rc;
}
