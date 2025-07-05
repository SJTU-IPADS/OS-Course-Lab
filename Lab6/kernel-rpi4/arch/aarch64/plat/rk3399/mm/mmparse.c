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

#include <arch/boot.h>
#include <machine.h>
#include <mm/mm.h>
#include <common/macro.h>
#include <arch/tools.h>

#define SGRF_BASE              0xff330000
#define GENMASK_32(h, l)       (((~0U) << (l)) & (~0U >> (32 - 1 - (h))))
#define SGRF_DDRRGN_CON0_16(n) ((n)*4)
#define SGRF_DDR_RGN_0_16_WMSK GENMASK_32(11, 0)
#define REG_MSK_SHIFT          16
#define WMSK_BIT(nr)           BIT((nr) + REG_MSK_SHIFT)
#define BIT_WITH_WMSK(nr)      (BIT(nr) | WMSK_BIT(nr))
#define SHIFT_U32(v, shift)    ((u32)(v) << (shift))
#define BITS_WMSK(msk, shift)  SHIFT_U32(msk, (shift) + REG_MSK_SHIFT)
#define BITS_WITH_WMASK(bits, msk, shift) \
        (SHIFT_U32(bits, shift) | BITS_WMSK(msk, shift))
static int secure_ddr_region(int rgn, paddr_t st, paddr_t ed)
{
        vaddr_t sgrf_base = phys_to_virt(SGRF_BASE);
        u32 st_mb = st / 0x100000;
        u32 ed_mb = ed / 0x100000;

        kinfo("protecting region %d: 0x%lx-0x%lx\n", rgn, st, ed);

        /* Set ddr region addr start */
        put32(sgrf_base + SGRF_DDRRGN_CON0_16(rgn),
              BITS_WITH_WMASK(st_mb, SGRF_DDR_RGN_0_16_WMSK, 0));

        /* Set ddr region addr end */
        put32(sgrf_base + SGRF_DDRRGN_CON0_16(rgn + 8),
              BITS_WITH_WMASK((ed_mb - 1), SGRF_DDR_RGN_0_16_WMSK, 0));

        put32(sgrf_base + SGRF_DDRRGN_CON0_16(16), BIT_WITH_WMSK(rgn));

        return 0;
}

void parse_mem_map(void *info)
{
        secure_ddr_region(1, get_tzdram_start(), get_tzdram_end());

        physmem_map_num = 1;
        physmem_map[0][0] = ROUND_UP((paddr_t)&img_end, PAGE_SIZE);
        physmem_map[0][1] = ROUND_DOWN(get_tzdram_end(), PAGE_SIZE);
        kinfo("[ChCore] physmem_map: [0x%lx, 0x%lx)\n",
              physmem_map[0][0],
              physmem_map[0][1]);
}
