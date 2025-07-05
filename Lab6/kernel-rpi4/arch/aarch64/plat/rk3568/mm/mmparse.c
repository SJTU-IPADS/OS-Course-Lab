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

#include <machine.h>
#include <mm/mm.h>
#include <common/macro.h>
#include <common/types.h>
#include <arch/tools.h>
#include <rk_atags.h>

static int secure_ddr_region(int rgn, paddr_t st, paddr_t ed)
{
        vaddr_t fw_base = phys_to_virt(0xFE200000);
        u32 bitmap;

        put32(fw_base + rgn * 4, (st >> 17) | ((ed - 1) >> 17 << 16));
        bitmap = get32(fw_base + 0x80);
        bitmap |= (1U << rgn);
        put32(fw_base + 0x80, bitmap);

        return 0;
}

void parse_mem_map(void *info)
{
        extern char img_end;

        secure_ddr_region(1, get_tzdram_start(), get_tzdram_start() + get_tzdram_size());

        physmem_map_num = 1;
        physmem_map[0][0] = ROUND_UP((paddr_t)&img_end, PAGE_SIZE);
        physmem_map[0][1] = ROUND_DOWN(get_tzdram_start() + get_tzdram_size(), PAGE_SIZE);
        kinfo("[ChCore] physmem_map: [0x%lx, 0x%lx)\n",
              physmem_map[0][0],
              physmem_map[0][1]);

        struct tag_tos_mem tag_tos_mem;
        memset(&tag_tos_mem, 0, sizeof(tag_tos_mem));
        memcpy(tag_tos_mem.tee_mem.name, "tee.mem", 8);
        tag_tos_mem.tee_mem.phy_addr = get_tzdram_start();
        tag_tos_mem.tee_mem.size = get_tzdram_size();
        tag_tos_mem.tee_mem.flags = 1;
        memcpy(tag_tos_mem.drm_mem.name, "drm.mem", 8);
        tag_tos_mem.drm_mem.phy_addr = 0;
        tag_tos_mem.drm_mem.size = 0;
        tag_tos_mem.drm_mem.flags = 0;
        tag_tos_mem.version = 65536;
        set_tos_mem_tag(&tag_tos_mem);
}
