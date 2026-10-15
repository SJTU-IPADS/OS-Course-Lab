/* Physical frames and the page table root, shared by every lab.
 *
 * Frames are handed out upwards from 1 MiB and never freed. The first
 * megabyte holds the root page table (0x1000) and the block device ring
 * (0x800).
 */
#include <stdio.h>
#include <string.h>

#include "mcpu.h"

uint64_t tlb_hits, tlb_misses, page_faults;

static uint64_t cr3;
static uint64_t next_frame = 0x00100000;

void mmu_set_cr3(uint64_t pa)
{
    cr3 = pa;
    mmu_tlb_flush();
}

uint64_t mmu_cr3(void)
{
    return cr3;
}

uint64_t mmu_alloc_frame(void)
{
    uint64_t f = next_frame;
    if (f + PAGE_SIZE > GUEST_RAM) {
        fprintf(stderr, "out of physical memory\n");
        return 0;
    }
    next_frame += PAGE_SIZE;
    /* A frame is never accessed before it is allocated, so no cache line
       can hold it yet and clearing it directly is safe. */
    uint8_t *p = dram_ptr(f, PAGE_SIZE);
    if (p)
        memset(p, 0, PAGE_SIZE);
    return f;
}
