/* The driver: power on, load a program, run it, print statistics.
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
 *     --interp          run on the interpreter of lab 1 instead of seq.c
 *     --pipe            run on the pipeline instead of seq.c
 *     --timeline FILE   stage timeline of every instruction (with --pipe)
 *     --no-forward      pipeline without forwarding
 *   lab 5
 *     --cache S,W       sets and ways of the cache (default 64,4)
 *     --miss N          MEM cycles of an access with a cache miss (default 10)
 *   lab 6
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
          " [--interp] [--pipe] [--timeline F] [--no-forward]"
#endif
#if LAB >= 5
          " [--cache S,W] [--miss N]"
#endif
#if LAB >= 6
          " [--disk F] [--ls]"
#endif
          " program [args...]\n", stderr);
}

static FILE *open_out(const char *path)
{
    if (!path)
        return NULL;
    if (!strcmp(path, "-"))
        return stdout;
    FILE *f = fopen(path, "w");
    if (!f)
        perror(path);
    return f;
}

int main(int argc, char **argv)
{
    const char *ram = "dram.img", *commit = NULL, *memtrace = NULL;
    int i, stats = 0, dis = 0;
#if LAB >= 2
    const char *timeline = NULL;
    int pipe = 0, interp = 0;
    uint64_t cycles = 0;
#endif
#if LAB >= 5
    unsigned sets = 64, ways = 4;
#endif
#if LAB >= 6
    const char *disk = "disk.img";
    int ls = 0;
#else
    const int ls = 0;
#endif

    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1]; ++i) {
        const char *a = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;
        if (!strcmp(a, "--ram") && v)             ram = argv[++i];
        else if (!strcmp(a, "--max") && v)        opt_max = strtoull(argv[++i], NULL, 0);
        else if (!strcmp(a, "--commit") && v)     commit = argv[++i];
        else if (!strcmp(a, "-v"))                opt_steps = 1;
        else if (!strcmp(a, "--stats"))           stats = 1;
        else if (!strcmp(a, "--memtrace") && v)   memtrace = argv[++i];
        else if (!strcmp(a, "--disasm"))          dis = 1;
#if LAB >= 2
        else if (!strcmp(a, "--interp"))          interp = 1;
        else if (!strcmp(a, "--pipe"))            pipe = 1;
        else if (!strcmp(a, "--timeline") && v)   timeline = argv[++i];
        else if (!strcmp(a, "--no-forward"))      opt_forward = 0;
#endif
#if LAB >= 5
        else if (!strcmp(a, "--cache") && v)      sscanf(argv[++i], "%u,%u", &sets, &ways);
        else if (!strcmp(a, "--miss") && v)       opt_miss = (unsigned)strtoul(argv[++i], NULL, 0);
#endif
#if LAB >= 6
        else if (!strcmp(a, "--disk") && v)       disk = argv[++i];
        else if (!strcmp(a, "--ls"))              ls = 1;
#endif
        else { usage(); return 2; }
    }
    if (i >= argc && !ls) { usage(); return 2; }
    if (dis)
        return disasm(argv[i], stdout) < 0;

    if (dram_open(ram, GUEST_RAM) < 0)
        return 1;

#if LAB >= 5
    /* Page table reads and writes go through the cache, so it must exist
       before loading. */
    cache_init(sets, ways);
#else
    cache_init(0, 0);
#endif

#if LAB >= 6
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
    if (loader_exec(&cpu, argv[i], argv + i, argc - i) < 0)
        return 1;
#if LAB >= 6
    if (has_disk && ramfs_mount() < 0)
        return 1;
#endif

    /* Loading and mounting are part of power-on and are not counted:
       write dirty lines back, then start over with an empty cache. */
    cache_flush();
#if LAB >= 5
    cache_init(sets, ways);
#endif
    mmu_tlb_flush();
    dram_reads = dram_writes = 0;
    tlb_hits = tlb_misses = page_faults = 0;
#if LAB >= 6
    blk_reads = blk_writes = 0;
#endif

    memtrace_out = open_out(memtrace);
    commit_out = open_out(commit);
    commit_start(&cpu);
#if LAB >= 2
    timeline_out = open_out(timeline);
    if (pipe)
        cycles = pipe_run(&cpu);
    else if (!interp)
        while (!cpu.halted) {
            if (opt_max && cpu.icount >= opt_max)
                break;
            if (cpu_step(&cpu) < 0)
                break;
        }
    else
#endif
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
    if (timeline_out && timeline_out != stdout)
        fclose(timeline_out);
    cache_flush();
    if (memtrace_out && memtrace_out != stdout)
        fclose(memtrace_out);
    memtrace_out = NULL;

    if (stats) {
        fprintf(stderr, "insns %lu  syscalls %lu  exit %lu\n",
                (unsigned long)cpu.icount, (unsigned long)syscall_count,
                (unsigned long)(cpu.r[RDI] & 0xff));
#if LAB >= 2
        if (pipe)
            fprintf(stderr, "pipe  cycles %lu  CPI %.3f\n", (unsigned long)cycles,
                    cpu.icount ? (double)cycles / (double)cpu.icount : 0.0);
#endif
#if LAB >= 4
        fprintf(stderr, "tlb   hits %lu  misses %lu  page faults %lu\n",
                (unsigned long)tlb_hits, (unsigned long)tlb_misses,
                (unsigned long)page_faults);
#endif
#if LAB >= 5
        uint64_t acc = cache_hits + cache_misses;
        fprintf(stderr,
                "cache accesses %lu  hits %lu  misses %lu  hit rate %.2f%%  writebacks %lu\n"
                "dram  line reads %lu  line writes %lu\n",
                (unsigned long)acc, (unsigned long)cache_hits,
                (unsigned long)cache_misses,
                acc ? 100.0 * (double)cache_hits / (double)acc : 0.0,
                (unsigned long)cache_writebacks,
                (unsigned long)dram_reads, (unsigned long)dram_writes);
#endif
#if LAB >= 6
        fprintf(stderr, "disk  reads %lu  writes %lu\n",
                (unsigned long)blk_reads, (unsigned long)blk_writes);
#endif
    }
    dram_close();
    return rc;
}
