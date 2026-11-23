/* Hardware: the MMU of the stand-in for lab 3, address translation kept in
 * a table on the host.
 *
 * The table plays the part of the page tables: no page tables in guest
 * memory, no TLB, and a translation costs no guest memory access. The
 * software that fills it, mmu_map, is frame/os/pagemap.c; it writes one
 * entry with pagemap_set, as mmu_map of lab 3 writes an entry of a page
 * table with pte_write. The interface is the one lab 3 implements, so the
 * loader and brk already call mmu_map and mmu_translate.
 */
#include <stdio.h>
#include <stdlib.h>

#include "mcpu.h"

#define SLOT_BITS 20                    /* open addressing, enough for 4 GiB: */
#define NSLOT (1u << SLOT_BITS)         /* a file mapped by mmap has many pages */

static struct { uint64_t vpn, pte; } slot[NSLOT];

static unsigned hash(uint64_t vpn)
{
    return (unsigned)((vpn * 0x9e3779b97f4a7c15ULL) >> (64 - SLOT_BITS));
}

void mmu_tlb_flush(void)
{
}

int pagemap_set(uint64_t vpn, uint64_t pte)
{
    for (unsigned idx = hash(vpn), probes = 0; probes < NSLOT;
         idx = (idx + 1) & (NSLOT - 1), ++probes)
        if (!(slot[idx].pte & PTE_P) || slot[idx].vpn == vpn) {
            slot[idx].vpn = vpn;
            slot[idx].pte = pte;
            return 0;
        }
    return -1;
}

int mmu_translate(uint64_t va, int acc, uint64_t *pa)
{
    uint64_t vpn = va / PAGE_SIZE;
    for (unsigned idx = hash(vpn), probes = 0; probes < NSLOT;
         idx = (idx + 1) & (NSLOT - 1), ++probes) {
        if (!(slot[idx].pte & PTE_P))
            break;
        if (slot[idx].vpn == vpn) {
            if (acc == MEM_WRITE && !(slot[idx].pte & PTE_W))
                break;
            *pa = (slot[idx].pte & ~0xfffULL) + va % PAGE_SIZE;
            return 0;
        }
    }
    ++page_faults;
    return -1;
}
