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

#ifndef COMMON_VARS_H
#define COMMON_VARS_H

/* Leave 8K space to per-CPU stack */
#define CPU_STACK_SIZE          4096
#define STACK_ALIGNMENT         16

#ifndef KBASE
#if __SIZEOF_POINTER__ == 4 /* 32-bit architecture */
#define KBASE		0xc0000000
#else /* 64-bit architecture */
#define KBASE		0xffffff0000000000
#endif
#endif

#endif /* COMMON_VARS_H */