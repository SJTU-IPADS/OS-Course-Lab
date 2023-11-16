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

.global __cp_begin
.hidden __cp_begin
.global __cp_end
.hidden __cp_end
.global __cp_cancel
.hidden __cp_cancel
.hidden __cancel
.global __syscall_cp_asm
.hidden __syscall_cp_asm
.type __syscall_cp_asm,%function
__syscall_cp_asm:
__cp_begin:
__cp_end:
__cp_cancel:
	/* Since we don't need this function in ChCore, just return -1 here. */
	set -1, %o0
	retl
	nop
	/* The following is the original implementation. */
	// ld [%o0], %g2
	// tst %g2
	// bne __cp_cancel
	// nop
	// mov %o1, %g1
	// mov %o2, %o0
	// mov %o3, %o1
	// mov %o4, %o2
	// mov %o5, %o3
	// ld [%sp + 92], %o4
	// ld [%sp + 96], %o5
	// ta SYSCALL_TRAP
// __cp_end:
	// retl
	// nop
// __cp_cancel:
	// b __cancel
