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
#include <common/types.h>
#include <arch/mmu.h>

#define TEEOS_CFG_OFFSET 0x8000

#define MAX_PROTECTED_REGIONS 16
#define GICR_MAX_NUM          8

typedef struct p_reg {
        u64 start;
        u64 end;
} p_region_t;

struct gic_config_t {
        char version;
        union {
                struct v2_t {
                        p_region_t dist;
                        p_region_t contr;
                } v2;
                struct v3_t {
                        p_region_t dist;
                        u32 redist_num;
                        u32 redist_stride;
                        p_region_t redist[GICR_MAX_NUM];
                } v3;
        };
};

struct platform_info {
        u64 plat_cfg_size;
        u64 phys_region_size;
        u64 phys_region_start;
        u64 uart_addr;
        struct gic_config_t gic_config;
        struct extend_datas_t {
                u64 extend_length;
                char extend_paras[0];
        } extend_datas;
};

static struct platform_info *teeos_cfg;

void teeos_cfg_init(paddr_t start_pa)
{
        teeos_cfg = (void *)phys_to_virt(start_pa - TEEOS_CFG_OFFSET);
}

paddr_t get_tzdram_start(void)
{
        return teeos_cfg->phys_region_start;
}

paddr_t get_tzdram_end(void)
{
        return teeos_cfg->phys_region_start + teeos_cfg->phys_region_size;
}

paddr_t get_gicd_base(void)
{
        return teeos_cfg->gic_config.v3.dist.start;
}

paddr_t get_uart_base(void)
{
        return teeos_cfg->uart_addr;
}
