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
#include <stdlib.h>
#include <chcore/defs.h>

// clang-format off
/**
 * When applying ASLR, we divide the TOTAL_ADDR_BIT(48bit/32bit) memory address
 * into 3 distinct parts, namely ASLR_HIGHER_BIT, ASLR_ENTROPY_BIT and
 * ASLR_ALIGN_BIT.
 *
 *   ASLR_HIGHER_BIT   ASLR_ENTROPY_BIT    ASLR_ALIGN_BIT
 *  |---------------|-------------------|------------------|
 *
 * ASLR_HIGHER_BIT is default to 4 or 8 for 64bit and 32bit systems respectively,
 *   in order to avoid possible conflict in virtual memory space.
 *   Without this constraint the address' significant bits would be randomized,
 *   for example, with a base addr 0x40000000, the random offset generated can be
 *   0x13210000, making the result 0x53210000, leading to potential conflict if
 *   the address range from 0x50000000 is reserved for other usage.
 * ASLR_ALIGN_BIT is default to 16, so that the memory address with be
 *   properly aligned. It seems sufficient to just set this to 12 so memory
 *   will be page-aligned, but we set it to a bigger default number in case some
 *   application expect beyond page-aligned memory address.
 * ASLR_ENTROPY_BIT is the only part that is to be randomized, ASLR will
 *   only change this part of the memory address
 */
// clang-format on

#define ASLR_HIGHER_BIT ((__SIZEOF_POINTER__ == 4) ? 8 : 4)
#define ASLR_ALIGN_BIT  (16)

#ifdef SV39
#define TOTAL_ADDR_BIT (35)
#else
#define TOTAL_ADDR_BIT  ((__SIZEOF_POINTER__ == 4) ? 32 : 48)
#endif

#if CHCORE_ASLR
#define ASLR_ENTROPY_BIT (TOTAL_ADDR_BIT - ASLR_ALIGN_BIT - ASLR_HIGHER_BIT)
#else
/* when ASLR_ENTROPY_BIT is set to 1, ASLR_RAND_OFFSET will always yield 0 */
#define ASLR_ENTROPY_BIT (1)
#endif

#define ASLR_RAND_OFFSET \
        ((rand() % ((1l << ASLR_ENTROPY_BIT) - 1)) << ASLR_ALIGN_BIT)
