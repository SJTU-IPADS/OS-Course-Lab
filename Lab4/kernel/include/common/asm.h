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

#ifndef COMMON_ASM_H
#define COMMON_ASM_H

#define ASM_EXTABLE_32(insn, fixup)	\
	.pushsection	.extable, "a";		\
	.align		2;				\
	.word		((insn));			\
	.word		((fixup));			\
	.popsection;

#define ASM_EXTABLE_64(insn, fixup)	\
	.pushsection	.extable, "a";		\
	.align		4;				\
	.quad		((insn));			\
	.quad		((fixup));			\
	.popsection;

/*
 *  clang-format will change '%function' into '% function',
 *  which cannot be understood by the compiler
 */
// clang-format off
#define BEGIN_FUNC(_name)        \
        .global _name;           \
        .type _name, %function; \
        _name:
// clang-format on

#define END_FUNC(_name) .size _name, .- _name

#define EXPORT(symbol) \
        .globl symbol; \
        symbol:

#define LOCAL_DATA(x) \
        .type x, 1;   \
        x:

#define DATA(x)    \
        .global x; \
        .hidden x; \
        LOCAL_DATA(x)

#define END_DATA(x) .size x, .- x

#endif /* COMMON_ASM_H */