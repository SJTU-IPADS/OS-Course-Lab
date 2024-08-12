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

/*
 *  Copyright (c) 2013, The Regents of the University of California (Regents).
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the Regents nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 *  SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 *  BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 *  HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#pragma once

/* TODO: Read it from device tree. */
#define PLAT_CPU_NUM 4

#ifndef __ASM__
#include <common/types.h>

/* Reference: riscv-pk bootloader/riscv64/riscv-pk/machine/encoding.h */

#define read_csr(reg)                                         \
	({                                                    \
		u64 __tmp;                          	      \
		asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
		__tmp;                                        \
	})

#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })

#define swap_csr(reg, val)                            \
	({                                            \
		u64 __tmp;                  	      \
		asm volatile("csrrw %0, " #reg ", %1" \
			     : "=r"(__tmp)            \
			     : "rK"(val));            \
		__tmp;                                \
	})

#define set_csr(reg, bit)                             \
	({                                            \
		u64 __tmp;                  	      \
		asm volatile("csrrs %0, " #reg ", %1" \
			     : "=r"(__tmp)            \
			     : "rK"(bit));            \
		__tmp;                                \
	})

#define clear_csr(reg, bit)                           \
	({                                            \
		u64 __tmp;                  	      \
		asm volatile("csrrc %0, " #reg ", %1" \
			     : "=r"(__tmp)            \
			     : "rK"(bit));            \
		__tmp;                                \
	})

#define rdtime()    read_csr(time)
#define rdcycle()   read_csr(cycle)
#define rdinstret() read_csr(instret)

void arch_cpu_init(void);

#endif
