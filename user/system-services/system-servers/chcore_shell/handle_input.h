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

#define FG_PROC_STACK_SIZE 32

void flush_buffer(void);
char shell_getchar(void);
void *shell_server(void *arg);
void send_cap_to_procmgr(cap_t cap);

#ifdef CHCORE_PLAT_RASPI3
#define UART_IRQ (57)
#elif defined(CHCORE_PLAT_LEON3) && !defined(CHCORE_QEMU)
#define UART_IRQ (2)
#elif defined(CHCORE_PLAT_BM3823)
#define UART_IRQ (3)
#endif

#ifdef UART_IRQ
void *handle_uart_irq(void *arg);
#else /* CHCORE_PLAT_RASPI3 */
void *other_plat_get_char(void *arg);
#endif /* CHCORE_PLAT_RASPI3 */
