/* Stand-in for lab 4: address translation kept in a table on the host.
 *
 * The interface is the one lab 4 implements, so the loader, brk and the
 * object loader of lab 3 already call mmu_map and mmu_translate. The table
 * lives outside the guest: no page tables in guest memory, no TLB, and a
 * translation costs no guest memory access.
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

int mmu_map(uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t vpn = va / PAGE_SIZE;
    for (unsigned i = hash(vpn), n = 0; n < NSLOT; i = (i + 1) & (NSLOT - 1), ++n)
        if (!(slot[i].pte & PTE_P) || slot[i].vpn == vpn) {
            slot[i].vpn = vpn;
            slot[i].pte = (pa & ~0xfffULL) | flags | PTE_P;
            return 0;
        }
    fprintf(stderr, "pagemap: table is full\n");
    return -1;
}

int mmu_translate(uint64_t va, int acc, uint64_t *pa)
{
    uint64_t vpn = va / PAGE_SIZE;
    for (unsigned i = hash(vpn), n = 0; n < NSLOT; i = (i + 1) & (NSLOT - 1), ++n) {
        if (!(slot[i].pte & PTE_P))
            break;
        if (slot[i].vpn == vpn) {
            if (acc == MEM_WRITE && !(slot[i].pte & PTE_W))
                break;
            *pa = (slot[i].pte & ~0xfffULL) + va % PAGE_SIZE;
            return 0;
        }
    }
    ++page_faults;
    return -1;
}
