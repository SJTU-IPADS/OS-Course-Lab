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

#if defined(CHCORE_PLAT_LEON3) || defined(CHCORE_PLAT_BM3823)
#define CACHE_MISS_ASI	0x01
#define MMU_FLUSH_ASI	0x18
#define MMU_REG_ASI	0x19
#define MMU_BYPASS_ASI	0x1C
#else
#define MMU_FLUSH_ASI	0x03
#define MMU_REG_ASI	0x04
#define MMU_BYPASS_ASI	0x20
#endif
