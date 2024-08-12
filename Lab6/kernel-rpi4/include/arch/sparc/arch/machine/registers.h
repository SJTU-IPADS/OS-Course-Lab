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

// The offset of some system registers.
#define NPC_OFFSET         0x210
#define PC_OFFSET          0x20c
#define Y_OFFSET           0x208
#define PSR_OFFSET         0x204
#define WIM_OFFSET         0x200

/* 
 * The following is general purpose registers of each window.
 * There are most 8 windows (W0 to W7),
 * we only need to save 16 registers in each window (I0 to I7 and L0 to L7)
 * since windows are overlapped.
 */
#define I7_OFFSET_W7 0x1fc
#define I6_OFFSET_W7 0x1f8
#define I5_OFFSET_W7 0x1f4
#define I4_OFFSET_W7 0x1f0
#define I3_OFFSET_W7 0x1ec
#define I2_OFFSET_W7 0x1e8
#define I1_OFFSET_W7 0x1e4
#define I0_OFFSET_W7 0x1e0

#define L7_OFFSET_W7 0x1dc
#define L6_OFFSET_W7 0x1d8
#define L5_OFFSET_W7 0x1d4
#define L4_OFFSET_W7 0x1d0
#define L3_OFFSET_W7 0x1cc
#define L2_OFFSET_W7 0x1c8
#define L1_OFFSET_W7 0x1c4
#define L0_OFFSET_W7 0x1c0

#define I7_OFFSET_W6 0x1bc
#define I6_OFFSET_W6 0x1b8
#define I5_OFFSET_W6 0x1b4
#define I4_OFFSET_W6 0x1b0
#define I3_OFFSET_W6 0x1ac
#define I2_OFFSET_W6 0x1a8
#define I1_OFFSET_W6 0x1a4
#define I0_OFFSET_W6 0x1a0

#define L7_OFFSET_W6 0x19c
#define L6_OFFSET_W6 0x198
#define L5_OFFSET_W6 0x194
#define L4_OFFSET_W6 0x190
#define L3_OFFSET_W6 0x18c
#define L2_OFFSET_W6 0x188
#define L1_OFFSET_W6 0x184
#define L0_OFFSET_W6 0x180

#define I7_OFFSET_W5 0x17c
#define I6_OFFSET_W5 0x178
#define I5_OFFSET_W5 0x174
#define I4_OFFSET_W5 0x170
#define I3_OFFSET_W5 0x16c
#define I2_OFFSET_W5 0x168
#define I1_OFFSET_W5 0x164
#define I0_OFFSET_W5 0x160

#define L7_OFFSET_W5 0x15c
#define L6_OFFSET_W5 0x158
#define L5_OFFSET_W5 0x154
#define L4_OFFSET_W5 0x150
#define L3_OFFSET_W5 0x14c
#define L2_OFFSET_W5 0x148
#define L1_OFFSET_W5 0x144
#define L0_OFFSET_W5 0x140

#define I7_OFFSET_W4 0x13c
#define I6_OFFSET_W4 0x138
#define I5_OFFSET_W4 0x134
#define I4_OFFSET_W4 0x130
#define I3_OFFSET_W4 0x12c
#define I2_OFFSET_W4 0x128
#define I1_OFFSET_W4 0x124
#define I0_OFFSET_W4 0x120

#define L7_OFFSET_W4 0x11c
#define L6_OFFSET_W4 0x118
#define L5_OFFSET_W4 0x114
#define L4_OFFSET_W4 0x110
#define L3_OFFSET_W4 0x10c
#define L2_OFFSET_W4 0x108
#define L1_OFFSET_W4 0x104
#define L0_OFFSET_W4 0x100

#define I7_OFFSET_W3 0xfc
#define I6_OFFSET_W3 0xf8
#define I5_OFFSET_W3 0xf4
#define I4_OFFSET_W3 0xf0
#define I3_OFFSET_W3 0xec
#define I2_OFFSET_W3 0xe8
#define I1_OFFSET_W3 0xe4
#define I0_OFFSET_W3 0xe0

#define L7_OFFSET_W3 0xdc
#define L6_OFFSET_W3 0xd8
#define L5_OFFSET_W3 0xd4
#define L4_OFFSET_W3 0xd0
#define L3_OFFSET_W3 0xcc
#define L2_OFFSET_W3 0xc8
#define L1_OFFSET_W3 0xc4
#define L0_OFFSET_W3 0xc0

#define I7_OFFSET_W2 0xbc
#define I6_OFFSET_W2 0xb8
#define I5_OFFSET_W2 0xb4
#define I4_OFFSET_W2 0xb0
#define I3_OFFSET_W2 0xac
#define I2_OFFSET_W2 0xa8
#define I1_OFFSET_W2 0xa4
#define I0_OFFSET_W2 0xa0

#define L7_OFFSET_W2 0x9c
#define L6_OFFSET_W2 0x98
#define L5_OFFSET_W2 0x94
#define L4_OFFSET_W2 0x90
#define L3_OFFSET_W2 0x8c
#define L2_OFFSET_W2 0x88
#define L1_OFFSET_W2 0x84
#define L0_OFFSET_W2 0x80

#define I7_OFFSET_W1 0x7c
#define I6_OFFSET_W1 0x78
#define I5_OFFSET_W1 0x74
#define I4_OFFSET_W1 0x70
#define I3_OFFSET_W1 0x6c
#define I2_OFFSET_W1 0x68
#define I1_OFFSET_W1 0x64
#define I0_OFFSET_W1 0x60

#define L7_OFFSET_W1 0x5c
#define L6_OFFSET_W1 0x58
#define L5_OFFSET_W1 0x54
#define L4_OFFSET_W1 0x50
#define L3_OFFSET_W1 0x4c
#define L2_OFFSET_W1 0x48
#define L1_OFFSET_W1 0x44
#define L0_OFFSET_W1 0x40

/*
 * Instead of 16 registers, only 8 registers (I0 to I7) in the first window are to be saved.
 * This is because L0 to L7 are local registers used by trap handler.
 */
#define I7_OFFSET_W0 0x3c
#define I6_OFFSET_W0 0x38
#define I5_OFFSET_W0 0x34
#define I4_OFFSET_W0 0x30
#define I3_OFFSET_W0 0x2c
#define I2_OFFSET_W0 0x28
#define I1_OFFSET_W0 0x24
#define I0_OFFSET_W0 0x20

/*
 * 8 global registers.
 * Though G0 (zero register) always yields 0, it still occupies one slot for alignment.
 */
#define G7_OFFSET 0x1c
#define G6_OFFSET 0x18
#define G5_OFFSET 0x14
#define G4_OFFSET 0x10
#define G3_OFFSET 0x0c
#define G2_OFFSET 0x08
#define G1_OFFSET 0x04
#define G0_OFFSET 0x00

#ifndef __ASM__
enum reg_type {
	G1 = 1,
	TLS = 7,
	I0_W0 = 8,
	I1_W0 = 9,
	I2_W0 = 10,
	I3_W0 = 11,
	I4_W0 = 12,
	I5_W0 = 13,
	SP = 14,
	I7_W0 = 15,
	WIM = 128,
	PSR = 129,
	Y = 130,
	PC = 131,
	NPC = 132
};

struct chcore_user_regs_struct {
	unsigned long g0, g1, g2, g3, g4, g5, g6, g7;
	unsigned long o0, o1, o2, o3, o4, o5, o6, o7;
	unsigned long l0, l1, l2, l3, l4, l5, l6, l7;
	unsigned long i0, i1, i2, i3, i4, i5, i6, i7;
	unsigned long y, psr, wim, tbr, pc, npc, fsr, csr;
};

struct psr {
	unsigned long
		impl	: 4,
		ver	: 4,
		icc	: 4,
		reserve	: 6,
		ec	: 1,
		ef	: 1,
		pil	: 4,
		s	: 1,
		ps	: 1,
		et	: 1,
		cwp	: 5;
};

extern char trapvector;

#define USER_SSR_OFFSET	0x80	/* Start of system state register */
#define USER_Y_OFFSET	0x80
#define USER_PSR_OFFSET	0x84
#define USER_WIM_OFFSET	0x88
#define USER_TBR_OFFSET	0x8c
#define USER_PC_OFFSET	0x90
#define USER_NPC_OFFSET	0x94
#define USER_FSR_OFFSET	0x98
#define USER_CSR_OFFSET	0x9c

#endif

#define REG_NUM			133
#define WINDOWS_NBR		8
/* The offset of restore_window_count in arch_exec_ctx_t */
#define RESTORE_CNT_OFFSET	0x214
/* The total size of arch_exec_ctx_t (aligned to double word) */
#define EXEC_CTX_SIZE		0x218

#define PSR_CWP_MASK	(0x1f)
#define PSR_ET_MASK	(1 << 5)
#define PSR_PS_MASK	(1 << 6)
#define PSR_S_MASK	(1 << 7)
#define PSR_PIL_MASK	(0xf00)
#define PSR_EF_MASK	(1 << 12)