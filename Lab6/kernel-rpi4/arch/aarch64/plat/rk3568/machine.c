/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <machine.h>

void teeos_cfg_init(paddr_t start_pa)
{
}

paddr_t get_tzdram_start(void)
{
        return 0x8400000;
}

paddr_t get_tzdram_size(void)
{
        return 0x3000000;
}

paddr_t get_gicd_base(void)
{
        return 0xfd400000;
}

paddr_t get_uart_base(void)
{
        return 0xfe660000;
}
