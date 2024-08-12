/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * Licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <common/macro.h>
#include "image.h"
#include "mailbox.h"
#include "boot.h"

typedef unsigned long u64;
typedef unsigned int u32;

#define EARLY_BUG_ON(expr)                                        \
        do {                                                      \
                if ((unlikely(expr))) {                           \
                        uart_send_string("BUG on (" #expr ")\n"); \
                        for (;;) {                                \
                        }                                         \
                }                                                 \
        } while (0)
/* First physical memory chunk address space: 0-1G */
#define PHYSMEM_START            (0x0UL)
#define L1_FIRST_PTE_END         (0x40000000UL)
#define L2_FIRST_TABLE_END       L1_FIRST_PTE_END
#define L2_FIRST_PTE_END         (0x200000UL)
#define L3_FIRST_TABLE_END       L2_FIRST_PTE_END
#define PERIPHERAL_BASE          (0xFC000000UL)
#define PERIPHERAL_END           (0xFF800000UL)
#define ARM_LOCAL_PERIPHERAL_END (0x100000000UL)
/* The number of entries in one page table page */
#define PTP_ENTRIES 512
/* The size of one page table page */
#define PTP_SIZE 4096
#define ALIGN(n) __attribute__((__aligned__(n)))
u64 boot_ttbr0_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l2_dram[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l2_peripheral[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr0_l3[PTP_ENTRIES] ALIGN(PTP_SIZE);

u64 boot_ttbr1_l0[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l1[PTP_ENTRIES] ALIGN(PTP_SIZE);

/* kernel .text segment is with different flags */
u64 boot_ttbr1_l2_0g_to_1g[PTP_ENTRIES] ALIGN(PTP_SIZE);
u64 boot_ttbr1_l3[PTP_ENTRIES] ALIGN(PTP_SIZE);

/*For the special 1G area including Peripheral Memory*/
u64 boot_ttbr1_l2_3g_to_4g[PTP_ENTRIES] ALIGN(PTP_SIZE);

#define IS_VALID (1UL << 0)
#define IS_TABLE (1UL << 1)
#define IS_PTE   (1UL << 1)

#define PXN            (0x1UL << 53) /* Privileged execute never */
#define UXN            (0x1UL << 54) /* Unprivileged execute never */
#define ACCESSED       (0x1UL << 10) /* Access flag */
#define NG             (0x1UL << 11) /* Mark as not global */
#define INNER_SHARABLE (0x3UL << 8) /* Sharebility */
#define NORMAL_MEMORY  (0x4UL << 2) /* Normal memory */
#define DEVICE_MEMORY  (0x0UL << 2) /* Device Memory*/
#define RDONLY_S       (0x2UL << 6) /* Read only */

#define NORMAL_TABLE_PARA (IS_TABLE | IS_VALID)
/* Never Execute, Accessable, Inner Sharable, Normal Memory */
#define NORMAL_CHUNK_PARA \
        (UXN | PXN | ACCESSED | INNER_SHARABLE | NORMAL_MEMORY | IS_VALID)
/* Never Execute, Accessable, Device Memory, Not Global*/
#define NORMAL_DEVICE_PARA \
        (UXN | PXN | ACCESSED | DEVICE_MEMORY | IS_VALID | NG)

#define SIZE_512G (512UL * 1024 * 1024 * 1024)
#define SIZE_1G   (1UL * 1024 * 1024 * 1024)
#define SIZE_2M   (2UL * 1024 * 1024)
#define SIZE_4K   (4UL * 1024)
#define SIZE_1M   (1UL * 1024 * 1024)

#define RPI_4B_TYPE 17

#define GET_BOARD_TYPE(board_revision) ((board_revision >> 4) & 0xFF)
#define GET_MEM_SIZE(board_revision)   ((256UL << ((board_revision >> 20UL) & 7UL)) * SIZE_1M)

#define MEM_3G_START     (3UL * SIZE_1G)
#define ENTRY_EACH_TABLE 512

#define GET_L0_INDEX(x) (((x) >> (12 + 9 + 9 + 9)) & 0x1ff)
#define GET_L1_INDEX(x) (((x) >> (12 + 9 + 9)) & 0x1ff)
#define GET_L2_INDEX(x) (((x) >> (12 + 9)) & 0x1ff)
#define GET_L3_INDEX(x) (((x) >> (12)) & 0x1ff)

#define INIT_STACK_SIZE (0x1000)
extern int boot_cpu_stack[PLAT_CPU_NUMBER][INIT_STACK_SIZE];

void uart_send_string(char *);

enum page_table_level {
        L0,
        L1,
        L2,
        L3,
};

enum virt_mem_type {
        HIGH_ADDRESS,
        LOW_ADDRESS,
};

/*
 * @size_each_entry: Output the size each entry point to. e.g. one L1 entry
 * points to 1GB memory. Output only if size_each_entry is not NULL.
 * @start/end_entry_idx: Output the pte index in the target page table.
 */
static void page_table_get_index(enum page_table_level level, u64 start_vaddr,
                                 u64 size, u64 *size_each_entry,
                                 u64 *start_entry_idx, u64 *end_entry_idx)
{
        u64 size_each_entry_tmp;
        switch (level) {
        case L0:
                size_each_entry_tmp = SIZE_512G;
                size = ROUND_UP(size, size_each_entry_tmp);
                *start_entry_idx = GET_L0_INDEX(start_vaddr);
                *end_entry_idx = *start_entry_idx + size / size_each_entry_tmp;
                break;
        case L1:
                size_each_entry_tmp = SIZE_1G;
                size = ROUND_UP(size, size_each_entry_tmp);
                *start_entry_idx = GET_L1_INDEX(start_vaddr);
                *end_entry_idx = *start_entry_idx + size / size_each_entry_tmp;
                break;
        case L2:
                size_each_entry_tmp = SIZE_2M;
                size = ROUND_UP(size, size_each_entry_tmp);
                *start_entry_idx = GET_L2_INDEX(start_vaddr);
                *end_entry_idx = *start_entry_idx + size / size_each_entry_tmp;
                break;
        case L3:
                size_each_entry_tmp = SIZE_4K;
                size = ROUND_UP(size, size_each_entry_tmp);
                *start_entry_idx = GET_L3_INDEX(start_vaddr);
                *end_entry_idx = *start_entry_idx + size / size_each_entry_tmp;
                break;
        default:
                uart_send_string("Invalid page table level.\r\n");
                EARLY_BUG_ON(1);
        }
        if (size_each_entry) {
                *size_each_entry = size_each_entry_tmp;
        }
}

static void fill_page_table_chunk(u64 *page_table, u64 start_phys, u64 size,
                                  enum virt_mem_type type,
                                  enum page_table_level level, u64 para)
{
        EARLY_BUG_ON(((level != L3) && (para & IS_TABLE))
                     || ((level == L3) && !(para & IS_PTE)));
        u64 start_entry_idx;
        u64 end_entry_idx;
        u64 size_each_entry;
        u64 start_virt;
        start_virt =
                (type == HIGH_ADDRESS ? KERNEL_VADDR + start_phys : start_phys);
        page_table_get_index(level,
                             start_virt,
                             size,
                             &size_each_entry,
                             &start_entry_idx,
                             &end_entry_idx);
        EARLY_BUG_ON(size > size_each_entry * ENTRY_EACH_TABLE);
        /*Detect alignment error*/
        EARLY_BUG_ON((start_phys & (size_each_entry - 1)));
        for (u64 idx = start_entry_idx; idx < end_entry_idx; idx++) {
                page_table[idx] =
                        (start_phys + (idx - start_entry_idx) * size_each_entry)
                        | para;
        }
}

/*Just fill one entry*/
static void fill_page_table_table(u64 *page_table, u64 target_table,
                                  u64 start_virt, u64 size,
                                  enum page_table_level level, u64 para)
{
        EARLY_BUG_ON((level == L3) || !(para & IS_TABLE));
        u64 start_entry_idx;
        u64 end_entry_idx;
        page_table_get_index(level,
                             start_virt,
                             size,
                             (u64 *)0,
                             &start_entry_idx,
                             &end_entry_idx);
        EARLY_BUG_ON(end_entry_idx - start_entry_idx != 1);
        page_table[start_entry_idx] = (target_table) | para;
}

static inline u64 get_board_revision()
{
        u32 mbox[7];
        // get the board's unique revision code with a mailbox call
        mbox[0] = 7 * 4; // length of the message
        mbox[1] = 0; // this is a request message
        mbox[2] = MBOX_TAG_GetBoardRevision; // get board revision
        mbox[3] = 4; // buffer size
        mbox[4] = 0; // request codes
        mbox[5] = 0; // clear output buffer
        mbox[6] = 0; // end tag
        /*
         * The following calculation refers to
         * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
         */
        unsigned int r =
                ((((unsigned int)((unsigned long)mbox) & ~0xF))
                 | (8 & 0xF));
        /* wait until we can write to the mailbox */
        do {
                asm volatile("nop");
        } while (*MBOX_STATUS & MBOX_FULL);
        /* write the address of our message to the mailbox with channel
         * identifier */
        *MBOX_WRITE = r;
        /* now wait for the response */
        while (1) {
                /* is there a response? */
                do {
                        asm volatile("nop");
                } while (*MBOX_STATUS & MBOX_EMPTY);
                /* is it a response to our message? */
                if (r == *MBOX_READ)
                        break;
        }
        return mbox[5];
}

u32 init_boot_pt(void)
{
        u32 board_revision;
        u64 physical_memory_end;

        /* TTBR1_EL1 0-1G */
        /* fill in page table L0 and L1, all maps in ttbr0 are not global. */
        fill_page_table_table(boot_ttbr0_l0,
                              (u64)boot_ttbr0_l1,
                              PHYSMEM_START,
                              L1_FIRST_PTE_END - PHYSMEM_START,
                              L0,
                              NORMAL_TABLE_PARA | NG);
        fill_page_table_table(boot_ttbr0_l1,
                              (u64)boot_ttbr0_l2_dram,
                              PHYSMEM_START,
                              L1_FIRST_PTE_END - PHYSMEM_START,
                              L1,
                              NORMAL_TABLE_PARA | NG);
        fill_page_table_table(boot_ttbr0_l2_dram,
                              (u64)boot_ttbr0_l3,
                              PHYSMEM_START,
                              L2_FIRST_PTE_END - PHYSMEM_START,
                              L2,
                              NORMAL_TABLE_PARA | NG);
        fill_page_table_table(boot_ttbr0_l1,
                              (u64)boot_ttbr0_l2_peripheral,
                              PERIPHERAL_BASE,
                              PERIPHERAL_END - PERIPHERAL_BASE,
                              L1,
                              NORMAL_TABLE_PARA | NG);

        /* map the area 0-2M, leaving kernel section init, excepting init
         * stacks, as not writable and executable */
        fill_page_table_chunk(boot_ttbr0_l3,
                              PHYSMEM_START,
                              L3_FIRST_TABLE_END - PHYSMEM_START,
                              LOW_ADDRESS,
                              L3,
                              NORMAL_CHUNK_PARA | NG | IS_PTE);
        fill_page_table_chunk(boot_ttbr0_l3,
                              (u64)(&img_start),
                              (u64)(&init_end) - (u64)(&img_start),
                              LOW_ADDRESS,
                              L3,
                              (NORMAL_CHUNK_PARA | NG | IS_PTE | RDONLY_S)
                                      & ~PXN);
        fill_page_table_chunk(boot_ttbr0_l3,
                              (u64)boot_cpu_stack,
                              PLAT_CPU_NUMBER * INIT_STACK_SIZE,
                              LOW_ADDRESS,
                              L3,
                              NORMAL_CHUNK_PARA | NG | IS_PTE);

        /* map the area 2M-1G which is left */
        fill_page_table_chunk(boot_ttbr0_l2_dram,
                              L2_FIRST_PTE_END,
                              L2_FIRST_TABLE_END - L2_FIRST_PTE_END,
                              LOW_ADDRESS,
                              L2,
                              (NORMAL_CHUNK_PARA | NG) & ~PXN);

        /* map the peripheral memory */
        fill_page_table_chunk(boot_ttbr0_l2_peripheral,
                              PERIPHERAL_BASE,
                              PERIPHERAL_END - PERIPHERAL_BASE,
                              LOW_ADDRESS,
                              L2,
                              NORMAL_DEVICE_PARA);

        /* TTBR1_EL1 0-1G */
        /* fill in page table L0 and L1 */
        fill_page_table_table(boot_ttbr1_l0,
                              (u64)boot_ttbr1_l1,
                              KERNEL_VADDR + PHYSMEM_START,
                              L1_FIRST_PTE_END - PHYSMEM_START,
                              L0,
                              NORMAL_TABLE_PARA);
        fill_page_table_table(boot_ttbr1_l1,
                              (u64)boot_ttbr1_l2_0g_to_1g,
                              KERNEL_VADDR + PHYSMEM_START,
                              L1_FIRST_PTE_END - PHYSMEM_START,
                              L1,
                              NORMAL_TABLE_PARA);
        fill_page_table_table(boot_ttbr1_l2_0g_to_1g,
                              (u64)boot_ttbr1_l3,
                              KERNEL_VADDR + PHYSMEM_START,
                              L2_FIRST_PTE_END - PHYSMEM_START,
                              L2,
                              NORMAL_TABLE_PARA);

        EARLY_BUG_ON((u64)(&_text_end) >= KERNEL_VADDR + SIZE_2M);
        /* _text_start & _text_end should be 4K aligned*/
        EARLY_BUG_ON((u64)(&_text_start) % SIZE_4K != 0
                     || (u64)(&_text_end) % SIZE_4K != 0);

        /* map area 0-2M, leaving kernel .text segment as executable and not
         * writable */
        fill_page_table_chunk(boot_ttbr1_l3,
                              PHYSMEM_START,
                              L3_FIRST_TABLE_END - PHYSMEM_START,
                              HIGH_ADDRESS,
                              L3,
                              NORMAL_CHUNK_PARA | IS_PTE);
        fill_page_table_chunk(boot_ttbr1_l3,
                              (u64)(&_text_start) - KERNEL_VADDR,
                              (u64)(&_text_end) - (u64)(&_text_start),
                              HIGH_ADDRESS,
                              L3,
                              (NORMAL_CHUNK_PARA | IS_PTE | RDONLY_S) & ~PXN);

        /* map the remain memory in area 0-1G (2M - 1G) */
        fill_page_table_chunk(boot_ttbr1_l2_0g_to_1g,
                              PHYSMEM_START + L2_FIRST_PTE_END,
                              L2_FIRST_TABLE_END - L2_FIRST_PTE_END,
                              HIGH_ADDRESS,
                              L2,
                              NORMAL_CHUNK_PARA);

        board_revision = get_board_revision();
        EARLY_BUG_ON(GET_BOARD_TYPE(board_revision) != RPI_4B_TYPE);
        physical_memory_end = GET_MEM_SIZE(board_revision);
        /* TTBR1_EL1 1G-END */
        /* map 1G-physical_memory_end */
        fill_page_table_chunk(boot_ttbr1_l1,
                              L1_FIRST_PTE_END,
                              physical_memory_end - L1_FIRST_PTE_END,
                              HIGH_ADDRESS,
                              L1,
                              NORMAL_CHUNK_PARA);

        /* since peripherals in area 3G-4G, overwrite the PTE pointing to a
         * chunk with one pointing to a L2 table */
        fill_page_table_table(boot_ttbr1_l1,
                              (u64)boot_ttbr1_l2_3g_to_4g,
                              KERNEL_VADDR + MEM_3G_START,
                              SIZE_1G,
                              L1,
                              NORMAL_TABLE_PARA);

        if (physical_memory_end >= PERIPHERAL_BASE) {
                /* map 3G-PERIPHERAL_BASE, if there is some usable memory in
                 * that area */
                fill_page_table_chunk(boot_ttbr1_l2_3g_to_4g,
                                      MEM_3G_START,
                                      PERIPHERAL_BASE - MEM_3G_START,
                                      HIGH_ADDRESS,
                                      L2,
                                      NORMAL_CHUNK_PARA);
        }

        /* TTBR1_EL1 Peripherals Memory */
        /* map peripherals memory, from PERIPHERAL_BASE to
         * ARM_LOCAL_PERIPHERAL_END */
        fill_page_table_chunk(boot_ttbr1_l2_3g_to_4g,
                              PERIPHERAL_BASE,
                              ARM_LOCAL_PERIPHERAL_END - PERIPHERAL_BASE,
                              HIGH_ADDRESS,
                              L2,
                              NORMAL_DEVICE_PARA);

        return board_revision;
}
