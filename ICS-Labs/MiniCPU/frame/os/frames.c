/* Software: the allocator of physical frames, shared by every lab.
 *
 * Frames are handed out upwards from 1 MiB and never freed. The first
 * megabyte holds the root page table (0x1000) and the block device ring
 * (0x800).
 */
#include <stdio.h>
#include <string.h>

#include "mcpu.h"

static uint64_t next_frame = 0x00100000;

uint64_t mmu_alloc_frame(void)
{
    uint64_t frame = next_frame;
    if (frame + PAGE_SIZE > GUEST_RAM) {
        fprintf(stderr, "out of physical memory\n");
        return 0;
    }
    next_frame += PAGE_SIZE;
    /* A frame is never accessed before it is allocated, so no cache line
       can hold it yet and clearing it directly is safe. */
    uint8_t *page = dram_ptr(frame, PAGE_SIZE);
    if (page)
        memset(page, 0, PAGE_SIZE);
    return frame;
}
