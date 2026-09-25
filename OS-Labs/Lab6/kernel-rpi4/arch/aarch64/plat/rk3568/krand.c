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

#include <arch/tools.h>
#include <arch/mmu.h>
#include <common/util.h>
#include <common/types.h>
#include <common/macro.h>

#define _SBF(s, v) ((v) << (s))
#define HIWORD_UPDATE(val, mask, shift) \
        ((val) << (shift) | (mask) << ((shift) + 16))

#define RK3568_TRNG_S 0xFE370000

/* start of CRYPTO V2 register define */
#define RK3568_TRNG_CTL               0x0400
#define RK3568_TRNG_64_BIT_LEN        _SBF(4, 0x00)
#define RK3568_TRNG_SLOWER_SOC_RING_0 _SBF(2, 0x01)
#define RK3568_TRNG_ENABLE            BIT(1)
#define RK3568_TRNG_START             BIT(0)
#define RK3568_TRNG_SAMPLE_CNT        0x0404
#define RK3568_TRNG_DOUT_0            0x0410

size_t krand(void)
{
        unsigned int reg_ctrl = 0;
        unsigned long trng_base = phys_to_virt(RK3568_TRNG_S);
        size_t random;

        put32(trng_base + RK3568_TRNG_SAMPLE_CNT, 1079);

        reg_ctrl |= RK3568_TRNG_64_BIT_LEN;
        reg_ctrl |= RK3568_TRNG_SLOWER_SOC_RING_0;
        reg_ctrl |= RK3568_TRNG_ENABLE;
        reg_ctrl |= RK3568_TRNG_START;
        put32(trng_base + RK3568_TRNG_CTL, HIWORD_UPDATE(reg_ctrl, 0xffff, 0));

        do {
                reg_ctrl = get32(trng_base + RK3568_TRNG_CTL);
        } while (reg_ctrl & RK3568_TRNG_START);

        memcpy(&random, (void *)trng_base + RK3568_TRNG_DOUT_0, sizeof(random));

        put32(trng_base + RK3568_TRNG_CTL, HIWORD_UPDATE(0, 0xffff, 0));

        return random;
}