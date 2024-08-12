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

#define TRAP_INST_ACCESS_EXCEPTION	0x01
#define TRAP_ILLEGAL_INST               0x02
#define TRAP_PRIV_INST                  0x03
#define TRAP_FP_DISABLED                0x04
#define TRAP_MEM_NOT_ALIGNED            0x07
#define TRAP_FP_EXECPTION               0x08
#define TRAP_DATA_ACCESS_EXCEPTION      0x09
#define TRAP_TAG_OVERFLOW               0x0a
#define TRAP_WATCHPOINT                 0x0b

#define TRAP_REG_ACCESS_ERR             0x20
#define TRAP_INST_ACCESS_ERR            0x21
#define TRAP_CP_DISABLED                0x24
#define TRAP_UNIMPL_FLUSH               0x25
#define TRAP_CP_EXECPTION               0x28
#define TRAP_DATA_ACCESS_ERR            0x29
#define TRAP_DIVISION_BY_ZERO           0x2a
#define TRAP_DATA_STORE_ERR             0x2b
#define TRAP_DATA_ACCESS_MMU_MISS       0x2c
#define TRAP_INST_ACCESS_MMU_MISS       0x3c

#define TRAP_FLUSH_WINDOW		0x83
#define TRAP_WINDOW_TEST		0x8a
#define TRAP_SYSCALL			0x90
#define TRAP_REVOKE_WINDOW              0x91
#define TRAP_BREAKPOINT                 0x92

#define TRAP_IRQ_BASE			0x10
#define TRAP_IRQ_END                    0x1f