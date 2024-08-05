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

/* This file references glibc/sysdeps/sparc/sparc32/__longjmp.S. */

/*
 * TODO: FLUSH_WINDOWS_TRAP is not implemented.
 * Currently setjmp and longjmp are not availeble.
 */

#include <bits/trap.h>

/* void longjmp(jmpbuf, retval) */
.global _longjmp
.global longjmp
.type   _longjmp,@function
.type   longjmp,@function
_longjmp:
longjmp:
	ld [%o0 + 4], %g3		/* %g3 = fp stored in jmpbuf (target_fp). */
	mov %o0, %g1		/* jmpbuf in %g1. */
	orcc %o1, %g0, %g2	/* retval in %g2 (return value of setjmp). */
	be,a 0f
	mov 1, %g2		/* Return value of setjmp can't be 0. */
0:
	xor %fp, %g3, %o0
	add %fp, 512, %o1
	andncc %o0, 0xfff, %o0	/* Is | fp - target_fp | < 4096 ? */
	bne .Lthread		/* Too much gap between fp and target_fp. */
	cmp %o1, %g3		/* fp + 512 < target_fp. */
	bl .Lthread		/* Too much gap between fp and target_fp. */

/* If fp < target_fp, we can restore windows to match them. */
.Lloop:
	cmp %fp, %g3		/* Have we reached the target frame? */
	bl,a .Lloop		/* fp < target_fp, restore and continue. */
	restore
	be,a .Lfound		/* fp == target_fp, found target frame. */
	ld [%g1], %o0		/* Load sp stored in jmpbuf. */
	/* If fp > target_fp, go through and do trap. */

/*
 * Unable (or too expensive) to match fp and target_fp by restoring windows.
 * We will instead do a "flush register windows trap" to save all the
 * register windows on their stack slots, and marks them all as invalid.
 */
.Lthread:
	save %sp, -96, %sp
	ta FLUSH_WINDOWS_TRAP
	ld [%g1 + 8], %i7	/* Load returen address stored in jmpbuf. */
	ld [%g1], %fp		/* Load sp stored in jmpbuf. */
	jmp %i7 + 8
	restore %g2, 0, %o0	/* Set return value. */

/* We match fp and target_fp successfully. */
.Lfound:
	mov %o0, %sp
	ld [%g1 + 8], %o0	/* Load returen address stored in jmpbuf. */
	jmp %o0 + 8
	mov %g2, %o0		/* Set return value. */
