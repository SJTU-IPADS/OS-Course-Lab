/* Hardware: the register CR3, which holds the physical address of the root
 * of the page tables, and the counters of the MMU.
 *
 * Writing CR3 empties the TLB, as on x86-64. The counters are those of
 * the line "tlb" of mini-cpu --stats: mmu_translate counts every lookup
 * and every page fault.
 */
#include "mcpu.h"

uint64_t tlb_hits, tlb_misses, page_faults;

static uint64_t cr3;

void mmu_set_cr3(uint64_t pa)
{
    cr3 = pa;
    mmu_tlb_flush();
}

uint64_t mmu_cr3(void)
{
    return cr3;
}
