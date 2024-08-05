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

#ifndef ARCH_AARCH64_ARCH_MACHINE_ESR_H
#define ARCH_AARCH64_ARCH_MACHINE_ESR_H

/* ESR_EL1 Exception Syndrome Register */
/* Exception Class, ESR bits[31:26].
 * A subset of exception is recoreded here. */
#define EC_MASK                 (BIT(6) - 1)
#define ESR_EL1_EC_SHIFT        26
#define GET_ESR_EL1_EC(esr_el1) (((esr_el1) >> ESR_EL1_EC_SHIFT) & EC_MASK)

// clang-format off
#define ESR_EL1_EC_UNKNOWN 0b000000
#define ESR_EL1_EC_WFI_WFE \
        0b000001 /* Trapped WFI or WFE instruction execution */
#define ESR_EL1_EC_ENFP \
        0b000111 /* Access to SVE, Advanced SIMD, or floating-point functionality */
#define ESR_EL1_EC_ILLEGAL_EXEC 0b001110 /* Illegal Execution state */
#define ESR_EL1_EC_SVC_32 \
        0b010001 /* SVC instruction execution in AArch32 state */
#define ESR_EL1_EC_SVC_64 \
        0b010101 /* SVC instruction execution in AArch64 state */
#define ESR_EL1_EC_MRS_MSR_64 \
        0b011000 /* Trapped MSR, MRS or System instruction execution in AArch64 state */
#define ESR_EL1_EC_IABT_LEL \
        0b100000 /* Instruction Abort from a lower Exception level) */
#define ESR_EL1_EC_IABT_CEL \
        0b100001 /* Instruction Abort from a current Exception level */
#define ESR_EL1_EC_PC_ALIGN 0b100010 /* PC alignment fault exception */
#define ESR_EL1_EC_DABT_LEL \
        0b100100 /* Data Abort from a lower Exception level */
#define ESR_EL1_EC_DABT_CEL \
        0b100101 /* Data Abort from a current Exception level */
#define ESR_EL1_EC_SP_ALIGN 0b100110 /* SP alignment fault exception */
#define ESR_EL1_EC_FP_32 \
        0b101000 /* Trapped floating-point exception taken from AArch32 state */
#define ESR_EL1_EC_FP_64 \
        0b101100 /* Trapped floating-point exception taken from AArch64 state */
#define ESR_EL1_EC_SError 0b101111 /* SERROR */
// clang-format on

/* ESR_EL1_ISS, ESR bits[24:0] is relay on the specific cases */

/* IFSC & DFSC */
#define FSC_MASK                 (BIT(6) - 1)
#define ESR_EL1_FSC_SHIFT        0
#define GET_ESR_EL1_FSC(esr_el1) (((esr_el1) >> ESR_EL1_FSC_SHIFT) & FSC_MASK)

/* Instruction Abort */
/* Instruction Fault Status Code, IFSC, ESR bits[5:0] */
#define IFSC_TRANS_FAULT_L0 0b000100
#define IFSC_TRANS_FAULT_L1 0b000101
#define IFSC_TRANS_FAULT_L2 0b000110
#define IFSC_TRANS_FAULT_L3 0b000111

#define IFSC_ACCESS_FAULT_L1 0b001001
#define IFSC_ACCESS_FAULT_L2 0b001010
#define IFSC_ACCESS_FAULT_L3 0b001011

#define IFSC_PERM_FAULT_L1 0b001101
#define IFSC_PERM_FAULT_L2 0b001110
#define IFSC_PERM_FAULT_L3 0b001111

/* Data Abort */
/* WnR, ESR bit[6]. Write not Read. The cause of data abort. */
#define DABT_BY_READ  0b0
#define DABT_BY_WRITE 0b1

#define WnR_MASK                 1
#define ESR_EL1_WnR_SHIFT        6
#define GET_ESR_EL1_WnR(esr_el1) (((esr_el1) >> ESR_EL1_WnR_SHIFT) & WnR_MASK)

/* Data Fault Status Code, DFSC, ESR bits[5:0] */
#define DFSC_TRANS_FAULT_L0 0b000100
#define DFSC_TRANS_FAULT_L1 0b000101
#define DFSC_TRANS_FAULT_L2 0b000110
#define DFSC_TRANS_FAULT_L3 0b000111

#define DFSC_ACCESS_FAULT_L1 0b001001
#define DFSC_ACCESS_FAULT_L2 0b001010
#define DFSC_ACCESS_FAULT_L3 0b001011

#define DFSC_PERM_FAULT_L1 0b001101
#define DFSC_PERM_FAULT_L2 0b001110
#define DFSC_PERM_FAULT_L3 0b001111

#ifdef CHCORE_KERNEL_VIRT

#define ESR_EL2_EC_UNKNOWN (0b000000)
#define ESR_EL2_EC_WFx     (0b000001)
/* Unallocated EC: 0b02 */
#define ESR_EL2_EC_CP15_32  (0b000011)
#define ESR_EL2_EC_CP15_64  (0b000100)
#define ESR_EL2_EC_CP14_MR  (0b000101)
#define ESR_EL2_EC_CP14_LS  (0b000110)
#define ESR_EL2_EC_FP_ASIMD (0b000111)
#define ESR_EL2_EC_CP10_ID  (0b001000) /* EL2 only */
#define ESR_EL2_EC_PAC      (0b001001) /* EL2 and above */
/* Unallocated EC: 0b0A - 0b0B */
#define ESR_EL2_EC_CP14_64 (0b001100)
/* Unallocated EC: 0b0d */
#define ESR_EL2_EC_ILL (0b001110)
/* Unallocated EC: 0b0F - 0b10 */
#define ESR_EL2_EC_SVC32 (0b010001)
#define ESR_EL2_EC_HVC32 (0b010010) /* EL2 only */
#define ESR_EL2_EC_SMC32 (0b010011) /* EL2 and above */
/* Unallocated EC: 0b14 */
#define ESR_EL2_EC_SVC64 (0b010101)
#define ESR_EL2_EC_HVC64 (0b010110) /* EL2 and above */
#define ESR_EL2_EC_SMC64 (0b010111) /* EL2 and above */
#define ESR_EL2_EC_SYS64 (0b011000)
#define ESR_EL2_EC_SVE   (0b011001)
/* Unallocated EC: 0b1A - 0b1F */
#define ESR_EL2_EC_IABT_LOW (0b100000)
#define ESR_EL2_EC_IABT_CUR (0b100001)
#define ESR_EL2_EC_PC_ALIGN (0b100010)
/* Unallocated EC: 0b23 */
#define ESR_EL2_EC_DABT_LOW (0b100100)
#define ESR_EL2_EC_DABT_CUR (0b100101)
#define ESR_EL2_EC_SP_ALIGN (0b100110)
/* Unallocated EC: 0b27 */
#define ESR_EL2_EC_FP_EXC32 (0b101000)
/* Unallocated EC: 0b29 - 0b2B */
#define ESR_EL2_EC_FP_EXC64 (0b101100)
/* Unallocated EC: 0b2D - 0b2E */
#define ESR_EL2_EC_SERROR      (0b101111)
#define ESR_EL2_EC_BREAKPT_LOW (0b110000)
#define ESR_EL2_EC_BREAKPT_CUR (0b110001)
#define ESR_EL2_EC_SOFTSTP_LOW (0b110010)
#define ESR_EL2_EC_SOFTSTP_CUR (0b110011)
#define ESR_EL2_EC_WATCHPT_LOW (0b110100)
#define ESR_EL2_EC_WATCHPT_CUR (0b110101)
/* Unallocated EC: 0b36 - 0b37 */
#define ESR_EL2_EC_BKPT32 (0b111000)
/* Unallocated EC: 0b39 */
#define ESR_EL2_EC_VECTOR32 (0b111010) /* EL2 only */
/* Unallocted EC: 0b3B */
#define ESR_EL2_EC_BRK64 (0b111100)
/* Unallocated EC: 0b3D - 0b3F */

#define ESR_EL2_EC_MAX (0b111111)

#define ESR_EL_EC_SHIFT (26)
#define ESR_EL_EC_MASK  ((0x3F) << ESR_EL_EC_SHIFT)
#define ESR_EL_EC(esr)  (((esr)&ESR_EL_EC_MASK) >> ESR_EL_EC_SHIFT)

/* Shared ISS field definitions for Data/Instruction aborts */
#define ESR_EL2_SET_SHIFT   (11)
#define ESR_EL2_SET_MASK    ((3) << ESR_EL2_SET_SHIFT)
#define ESR_EL2_FnV_SHIFT   (10)
#define ESR_EL2_FnV         ((1) << ESR_EL2_FnV_SHIFT)
#define ESR_EL2_EA_SHIFT    (9)
#define ESR_EL2_EA          ((1) << ESR_EL2_EA_SHIFT)
#define ESR_EL2_S1PTW_SHIFT (7)
#define ESR_EL2_S1PTW       ((1) << ESR_EL2_S1PTW_SHIFT)

/* Shared ISS fault status code(IFSC/DFSC) for Data/Instruction aborts */
#define ESR_EL2_FSC        (0x3F)
#define ESR_EL2_FSC_TYPE   (0x3C)
#define ESR_EL2_FSC_EXTABT (0x10)
#define ESR_EL2_FSC_SERROR (0x11)
#define ESR_EL2_FSC_ACCESS (0x08)
#define ESR_EL2_FSC_FAULT  (0x04)
#define ESR_EL2_FSC_PERM   (0x0C)

/* ISS field definitions for Data Aborts */
#define ESR_EL2_ISV_SHIFT  (24)
#define ESR_EL2_ISV        ((1) << ESR_EL2_ISV_SHIFT)
#define ESR_EL2_SAS_SHIFT  (22)
#define ESR_EL2_SAS        ((3) << ESR_EL2_SAS_SHIFT)
#define ESR_EL2_SSE_SHIFT  (21)
#define ESR_EL2_SSE        ((1) << ESR_EL2_SSE_SHIFT)
#define ESR_EL2_SRT_SHIFT  (16)
#define ESR_EL2_SRT_MASK   ((0x1F) << ESR_EL2_SRT_SHIFT)
#define ESR_EL2_SF_SHIFT   (15)
#define ESR_EL2_SF         ((1) << ESR_EL2_SF_SHIFT)
#define ESR_EL2_AR_SHIFT   (14)
#define ESR_EL2_AR         ((1) << ESR_EL2_AR_SHIFT)
#define ESR_EL2_VNCR_SHIFT (13)
#define ESR_EL2_VNCR       ((1) << ESR_EL2_VNCR_SHIFT)
#define ESR_EL2_CM_SHIFT   (8)
#define ESR_EL2_CM         ((1) << ESR_EL2_CM_SHIFT)
#define ESR_EL2_WNR_SHIFT  (6)
#define ESR_EL2_WNR        ((1) << ESR_EL2_WNR_SHIFT)

/* ISS field definitions for exceptions taken in to Hyp */
#define ESR_EL2_CV           ((1) << 24)
#define ESR_EL2_COND_SHIFT   (20)
#define ESR_EL2_COND_MASK    ((0xF) << ESR_EL2_COND_SHIFT)
#define ESR_EL2_WFx_ISS_TI   ((1) << 0)
#define ESR_EL2_WFx_ISS_WFI  ((0) << 0)
#define ESR_EL2_WFx_ISS_WFE  ((1) << 0)
#define ESR_EL2_xVC_IMM_MASK ((1 << 16) - 1)

#endif /* CHCORE_KERNEL_VIRT */

#endif /* ARCH_AARCH64_ARCH_MACHINE_ESR_H */