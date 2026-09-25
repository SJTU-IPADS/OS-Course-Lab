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

#ifndef MM_MM_H
#define MM_MM_H
#include <common/macro.h>

#define PAGE_SIZE (0x1000)

#define phys_to_virt(x)                   \
        ({                                \
                BUG("not implemented\n"); \
                0ull;                     \
        })
#define virt_to_phys(x)                   \
        ({                                \
                BUG("not implemented\n"); \
                0ull;                     \
        })

#endif /* MM_MM_H */