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

/*
 * GICv3 cpu interface registers.
 * Refers to GICv3 Architecture Manual
 */

/* ICC_CTLR_EL1 */
#define ICC_CTLR_EL1_EOImode_SHIFT   1
#define ICC_CTLR_EL1_CBPR_SHIFT      0
#define ICC_CTLR_EL1_PRIbits_SHIFT   8
#define ICC_CTLR_EL1_PRIbits_MASK    0x700

/* ICC_SRE_EL1 */
#define ICC_SRE_EL1_SRE              (1U << 0)

/* SGI registers */
#define ICC_SGI1R_SGI_ID_SHIFT         24
#define ICC_SGI1R_AFFINITY_1_SHIFT     16
#define ICC_SGI1R_AFFINITY_2_SHIFT     32
#define ICC_SGI1R_AFFINITY_3_SHIFT     48
#define ICC_SGI1R_RS_SHIFT             44

#define stringify(x)    #x
#define ICC_IGRPEN1_EL1 stringify(S3_0_C12_C12_7)
#define ICC_SRE_EL1     stringify(S3_0_C12_C12_5)
#define ICC_CTLR_EL1    stringify(S3_0_C12_C12_4)
#define ICC_BPR1_EL1    stringify(S3_0_C12_C12_3)
#define ICC_EOIR1_EL1   stringify(S3_0_C12_C12_1)
#define ICC_IAR1_EL1    stringify(S3_0_C12_C12_0)
#define ICC_SGI1R_EL1   stringify(S3_0_C12_C11_5)
#define ICC_DIR_EL1     stringify(S3_0_C12_C11_1)
#define ICC_AP1R0_EL1   stringify(S3_0_C12_C9_0)
#define ICC_AP1R1_EL1   stringify(S3_0_C12_C9_1)
#define ICC_AP1R2_EL1   stringify(S3_0_C12_C9_2)
#define ICC_AP1R3_EL1   stringify(S3_0_C12_C9_3)
#define ICC_AP0R0_EL1   stringify(S3_0_C12_C8_4)
#define ICC_AP0R1_EL1   stringify(S3_0_C12_C8_5)
#define ICC_AP0R2_EL1   stringify(S3_0_C12_C8_6)
#define ICC_AP0R3_EL1   stringify(S3_0_C12_C8_7)
#define ICC_PMR_EL1     stringify(S3_0_C4_C6_0)

#define read_sys_reg(sys_reg)                                              \
	({                                                                 \
		u32 val;                                                   \
		asm volatile("mrs %0, " sys_reg : "=r"(val) : : "memory"); \
		val;                                                       \
	})

#define write_sys_reg(sys_reg, val) \
	asm volatile("msr " sys_reg ", %0" : : "r"((val)) : "memory")

static inline bool gicv3_enable_sre(void)
{
	u32 val;

	val = read_sys_reg(ICC_SRE_EL1);
	if (val & ICC_SRE_EL1_SRE)
		return true;

	val |= ICC_SRE_EL1_SRE;
	write_sys_reg(ICC_SRE_EL1, val);
	isb();
	val = read_sys_reg(ICC_SRE_EL1);

	return !!(val & ICC_SRE_EL1_SRE);
}

#define MPIDR_AFFINITY_LEVEL(mpidr, level) \
	((mpidr >> (level * 8)) & 0xFF)

#define MPIDR_TO_SGI_AFFINITY(cluster_id, level) \
	(MPIDR_AFFINITY_LEVEL(cluster_id, level) \
	 << ICC_SGI1R_AFFINITY_##level##_SHIFT)

#define MPIDR_RS(mpidr)                (((mpidr)&0xF0UL) >> 4)
#define MPIDR_TO_SGI_RS(mpidr)         (MPIDR_RS(mpidr) << ICC_SGI1R_RS_SHIFT)
#define MPIDR_TO_SGI_CLUSTER_ID(mpidr) ((mpidr) & ~0xFUL)

static inline void gicv3_set_routing(u64 cpumask, void *rout_reg)
{
	put32((u64)rout_reg, (u32)cpumask);
	put32((u64)(rout_reg + 4), (u32)(cpumask >> 32));
}
