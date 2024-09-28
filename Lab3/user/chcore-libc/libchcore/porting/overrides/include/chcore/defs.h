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

#include <uapi/memory.h>
#include <uapi/syscall_num.h>
#include <uapi/cache.h>
#include <uapi/ptrace.h>


#ifdef CHCORE_PLAT_VISIONFIVE2
#define SV39
#endif

/* Use -1 instead of 0 (NULL) since 0 is used as the ending mark of envp */
#define ENVP_NONE_PMOS (-1)
#define ENVP_NONE_CAPS (-1)

/* ChCore custom auxiliary vector types */
#define AT_CHCORE_PMO_CNT 0xCC00
#define AT_CHCORE_PMO_MAP_ADDR 0xCC01
#define AT_CHCORE_CAP_CNT 0xCC02
#define AT_CHCORE_CAP_VAL 0xCC03
#define AT_CHCORE_LIBC_PMO_CAP 0xCC04

/*
 * Used as a token which is added at the beginnig of the path name
 * during booting a system server. E.g., "kernel-server/fsm.srv"
 *
 * This is simply for preventing users unintentionally
 * run system servers in Shell. That is meaningless.
 */
#define KERNEL_SERVER "kernel-server"

#define NO_AFF       (-1)
#define NO_ARG       (0)
#define PASSIVE_PRIO (-1)

/* virtual memory rights */
#define VM_READ   (1 << 0)
#define VM_WRITE  (1 << 1)
#define VM_EXEC   (1 << 2)
#define VM_FORBID (0)

/* a thread's own cap_group */
#define SELF_CAP 0

#define MAX_PRIO	255
#define MIN_PRIO	1
#ifndef CHCORE_OPENTRUSTEE
#define DEFAULT_PRIO	MIN_PRIO
#else /* CHCORE_OPENTRUSTEE */
#define DEFAULT_PRIO	10
#endif /* CHCORE_OPENTRUSTEE */

#if __SIZEOF_POINTER__ == 4
#define CHILD_THREAD_STACK_BASE (0x50800000UL)
#define CHILD_THREAD_STACK_SIZE (0x200000UL)
#elif defined(SV39)
#define CHILD_THREAD_STACK_BASE (0x3000800000UL)
#define CHILD_THREAD_STACK_SIZE (0x800000UL)
#else
#define CHILD_THREAD_STACK_BASE (0x500000800000UL)
#define CHILD_THREAD_STACK_SIZE (0x800000UL)
#endif
#define CHILD_THREAD_PRIO (DEFAULT_PRIO)

#if __SIZEOF_POINTER__ == 4
#define MAIN_THREAD_STACK_BASE (0x50000000UL)
#define MAIN_THREAD_STACK_SIZE (0x200000UL)
#elif defined(SV39)
#define MAIN_THREAD_STACK_BASE (0x3000000000UL)
#define MAIN_THREAD_STACK_SIZE (0x800000UL)
#else
#define MAIN_THREAD_STACK_BASE (0x500000000000UL)
#define MAIN_THREAD_STACK_SIZE (0x800000UL)
#endif
#define MAIN_THREAD_PRIO (DEFAULT_PRIO)

#define IPC_PER_SHM_SIZE (0x1000)

#define ROUND_UP(x, n)   (((x) + (n)-1) & ~((n)-1))
#define ROUND_DOWN(x, n) ((x) & ~((n)-1))

#define unused(x) ((void)(x))

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

/**
 * read(2): 
 * On Linux, read() (and similar system calls) will transfer at most
 * 0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
 * actually transferred.  (This is true on both 32-bit and 64-bit
 * systems.)
 */
#define READ_SIZE_MAX (0x7ffff000)

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
