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

/* Supervisor Status Register, sstatus. */
#define SSTATUS_SPP  (1L << 8)          // Previous mode, 1 = Supervisor, 0 = User
#define SSTATUS_SPIE (1L << 5)          // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4)          // User Previous Interrupt Enable
#define SSTATUS_SIE  (1L << 1)          // Supervisor Interrupt Enable
#define SSTATUS_UIE  (1L << 0)          // User Interrupt Enable
#define SSTATUS_SUM  (1L << 18)         // Supervisor User Memory Access
#define SSTATUS_MXR  (1L << 19)         // Make eXecutable Readable

/* Floating-point related bits in sstatus. */
#define SSTATUS_FS              (3L << 13)
#define SSTATUS_FS_OFF          (0L << 13)
#define SSTATUS_FS_INITIAL      (1L << 13)
#define SSTATUS_FS_CLEAN        (2L << 13)
#define SSTATUS_FS_DIRTY        (3L << 13)
#define SSTATUS_SD              (1L << 63)

/* Supervisor Interrupt Enable Register, sie. */
#define SIE_SSIE        (1L << 1)      // software interrupt
#define SIE_STIE        (1L << 5)      // timer interrupt
#define SIE_SEIE        (1L << 9)      // external interrupt
#define SIE_UTIE        (1L << 4)      // user timer interrupt

/* Supervisor Cause Register, scause. */
#define SCAUSE_ASYN_IRQ         (1L << 63)

/* Supervisor Interrupt Pending Register, sip. */
#define SIP_USIP        (1L << 0)
#define SIP_SSIP        (1L << 1)
#define SIP_UTIP        (1L << 4)
#define SIP_STIP        (1L << 5)
#define SIP_UEIP        (1L << 8)
#define SIP_SEIP        (1L << 9)

#ifndef __ASM__
/* Types of the registers */
enum reg_type {
        // x0 need not to be saved and restored.
        X1 = 0,         // return address
        X2 = 1,         // stack pointer
        X3 = 2,         // global pointer
        // x4, aka tp, is saved in tls_base_reg.
        X5 = 3,         // t0, temporary/alternate link register
        X6 = 4,         // t1, temporary
        X7 = 5,         // t2, temporary
        X8 = 6,         // s0, fp
        X9 = 7,         // s1, saved register
        X10 = 8,        // a0, function argument, return value
        X11 = 9,       // a1, function argument, return value, some implementation of RISC-V
                        // support return two values.
        X12 = 10,       // a2, function argument
        X13 = 11,       // a3, function argument
        X14 = 12,       // a4, function argument
        X15 = 13,       // a5, function argument
        X16 = 14,       // a6, function argument
        X17 = 15,       // a7, function argument
        X18 = 16,       // s2, saved register
        X19 = 17,       // s3, saved register
        X20 = 18,       // s4, saved register
        X21 = 19,       // s5, saved register
        X22 = 20,       // s6, saved register
        X23 = 21,       // s7, saved register
        X24 = 22,       // s8, saved register
        X25 = 23,       // s9, saved register
        X26 = 24,       // s10, saved register
        X27 = 25,       // s11, saved register
        X28 = 26,       // t3, temporary
        X29 = 27,       // t4, temporary
        X30 = 28,       // t5, temporary
        X31 = 29,       // t6, temporary
        SEPC = 30,      // pc
        SSTATUS = 31,   // sstatus contains FPU status which need save & restoration
};

struct chcore_user_regs_struct {
};
#endif

#define REG_NUM 32

/* offset in tls_base_reg */
#define TLS_TP  0

/* In bytes */
#define SZ_U64              8
#define ARCH_EXEC_CONT_SIZE (REG_NUM * SZ_U64)
