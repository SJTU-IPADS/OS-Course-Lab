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

#if defined(CHCORE_PLAT_RASPI3)
#define MMIO_BASE       0x3F000000
#elif defined(CHCORE_PLAT_RASPI4)
#define MMIO_BASE	0xFE000000
#else
#error "Unsupported platform"
#endif

#define GPFSEL0         ((volatile unsigned int*)(MMIO_BASE + 0x00200000))
#define GPFSEL1         ((volatile unsigned int*)(MMIO_BASE + 0x00200004))
#define GPFSEL2         ((volatile unsigned int*)(MMIO_BASE + 0x00200008))
#define GPFSEL3         ((volatile unsigned int*)(MMIO_BASE + 0x0020000C))
#define GPFSEL4         ((volatile unsigned int*)(MMIO_BASE + 0x00200010))
#define GPFSEL5         ((volatile unsigned int*)(MMIO_BASE + 0x00200014))
#define GPSET0          ((volatile unsigned int*)(MMIO_BASE + 0x0020001C))
#define GPSET1          ((volatile unsigned int*)(MMIO_BASE + 0x00200020))
#define GPCLR0          ((volatile unsigned int*)(MMIO_BASE + 0x00200028))
#define GPLEV0          ((volatile unsigned int*)(MMIO_BASE + 0x00200034))
#define GPLEV1          ((volatile unsigned int*)(MMIO_BASE + 0x00200038))
#define GPEDS0          ((volatile unsigned int*)(MMIO_BASE + 0x00200040))
#define GPEDS1          ((volatile unsigned int*)(MMIO_BASE + 0x00200044))
#define GPHEN0          ((volatile unsigned int*)(MMIO_BASE + 0x00200064))
#define GPHEN1          ((volatile unsigned int*)(MMIO_BASE + 0x00200068))
#define GPPUD           ((volatile unsigned int*)(MMIO_BASE + 0x00200094))
#define GPPUDCLK0       ((volatile unsigned int*)(MMIO_BASE + 0x00200098))
#define GPPUDCLK1       ((volatile unsigned int*)(MMIO_BASE + 0x0020009C))
