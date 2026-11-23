/* Software: mmu_map of the stand-in for lab 3, which fills the table of
 * frame/cpu/pagemap.c instead of page tables in guest memory.
 *
 * The entry of a page is (pa & ~0xfff) | flags | PTE_P, the entry of
 * level 0 that mmu_map of lab 3 writes.
 */
#include <stdio.h>

#include "mcpu.h"

int mmu_map(uint64_t va, uint64_t pa, uint64_t flags)
{
    if (pagemap_set(va / PAGE_SIZE, (pa & ~0xfffULL) | flags | PTE_P) == 0)
        return 0;
    fprintf(stderr, "pagemap: table is full\n");
    return -1;
}
