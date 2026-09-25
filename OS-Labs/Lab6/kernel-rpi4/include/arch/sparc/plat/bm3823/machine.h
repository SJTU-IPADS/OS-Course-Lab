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

#pragma once

#define PLAT_CPU_NUM		1

#define CACHE_CTRL_REG_PHYS     (0x80000014UL)

#define CACHE_CTRL_REG		(0xF0000014UL)
#define CACHE_CTRL_DS_MASK	(0x1 << 23)
#define CACHE_CTRL_FDFI_MASK	(0x3 << 21)
#define CACHE_CTRL_IB_MASK	(0x1 << 16)
#define CACHE_CTRL_IPDP_MASK	(0x3 << 14)
#define CACHE_CTRL_EN_MASK	(0xF)