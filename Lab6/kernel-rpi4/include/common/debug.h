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

#ifndef COMMON_DEBUG_H
#define COMMON_DEBUG_H

#define OFF 0
#define ON  1

/*
 * When ENABLE_HOOKING_SYSCALL is ON,
 * ChCore will print the syscall number before invoking the handler.
 * Please refer to kernel/syscall/syscall.c.
 */
#define ENABLE_HOOKING_SYSCALL OFF

/*
 * When ENABLE_DETECTING_DOUBLE_FREE_IN_SLAB is ON,
 * ChCore will check each free operation in the slab allocator.
 * Please refer to kernel/mm/slab.c.
 */
#define ENABLE_DETECTING_DOUBLE_FREE_IN_SLAB OFF

/*
 * When ENABLE_BACKTRACE_FUNC is ON,
 * You can use the backtrace function to trace the invoking pipe line.
 * Please refer to kernel/arch/aarch64/backtrace/backtrace.c.
 */
#define ENABLE_BACKTRACE_FUNC ON

/*
 * When ENABLE_MEMORY_USAGE_COLLECTING is ON,
 * ChCore can collect the memory allocated but not freed by a cap group.
 * Note that this is only a debug tool without guaranteeing any semantic.
 */
#define ENABLE_MEMORY_USAGE_COLLECTING OFF

/* Ease-of-debug: Make different user processes invoke print in sequence. */
#define ENABLE_PRINT_LOCK ON

#endif /* COMMON_DEBUG_H */
