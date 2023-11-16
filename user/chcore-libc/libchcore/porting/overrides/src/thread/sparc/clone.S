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

#include <bits/trap.h>

.global __clone
.hidden __clone
.type   __clone,%function
__clone:
	/* Since we don't need this function in ChCore, just return -1 here. */
	set -1, %o0
	retl
	nop
	/* The following is the original implementation. */
	// save %sp, -96, %sp
	// mov %i0, %g2
	// sub %i1, 96, %o1
	// mov %i2, %o0
	// mov %i3, %g3
	// mov %i4, %o2
	// mov %i5, %o3
	// ld [%fp + 92], %o4

	// set 220, %g1 /* SYS_clone */
	// ta SYSCALL_TRAP

	// tst %o1
	// bne 1f
	// nop
	// jmpl %i7 + 8, %g0
	// restore %o0, %g0, %o0
// 1:
	// mov %g0, %fp /* terminate backtrace */
	// call %g2
	// mov %g3, %o0
	// set 93, %g1 /* SYS_exit */
	// ta SYSCALL_TRAP
