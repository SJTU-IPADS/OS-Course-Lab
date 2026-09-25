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

#ifndef BOOT_H
#define BOOT_H

/* defined in tools.S */
void el1_mmu_activate(void);
/* defined in mmu.c */
void init_kernel_pt(void);

/* defined in arch/aarch64/head.S */
void start_kernel(void *boot_flag);
void secondary_cpu_boot(int cpuid);

extern char _bss_start;
extern char _bss_end;

extern char img_start;
extern char init_end;

extern char _text_start;
extern char _text_end;

#define PLAT_CPU_NUMBER		4

#define ALIGN(n)		__attribute__((__aligned__(n)))

#endif /* BOOT_H */
