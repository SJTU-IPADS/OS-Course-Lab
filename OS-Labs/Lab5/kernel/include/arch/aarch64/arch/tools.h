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

#ifndef ARCH_AARCH64_ARCH_TOOLS_H
#define ARCH_AARCH64_ARCH_TOOLS_H

void flush_dcache_area(unsigned long addr, unsigned long size);
void enable_irq(void);
void disable_irq(void);
void enable_uart_irq(int irqno);
void uart_irq_handler(void);
void put32(unsigned long addr, unsigned int data);
unsigned int get32(unsigned long addr);

#endif /* ARCH_AARCH64_ARCH_TOOLS_H */