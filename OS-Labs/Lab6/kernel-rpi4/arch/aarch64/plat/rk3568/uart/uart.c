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

#include <machine.h>
#include <common/types.h>
#include <arch/mmu.h>
#include <arch/tools.h>

/* uart register defines */
#define UART_RHR 0x0
#define UART_THR 0x0
#define UART_IER 0x4
#define UART_ISR 0x8
#define UART_FCR 0x8
#define UART_LCR 0xc
#define UART_MCR 0x10
#define UART_LSR 0x14
#define UART_MSR 0x18
#define UART_SPR 0x1c

/* uart status register bits */
#define LSR_TEMT  0x40 /* Transmitter empty */
#define LSR_THRE  0x20 /* Transmit-hold-register empty */
#define LSR_EMPTY (LSR_TEMT | LSR_THRE)
#define LSR_DR    0x01 /* DATA Ready */

static void serial8250_uart_flush(vaddr_t base)
{
        u32 state;

        /* Wait until transmit FIFO is empty */
        do {
                state = get32(base + UART_LSR);
        } while ((state & LSR_EMPTY) != LSR_EMPTY);
}

static void serial8250_uart_send(vaddr_t base, int c)
{
        serial8250_uart_flush(base);
        put32(base + UART_THR, c);
}

void uart_init(void)
{
        /* Do nothing */
}

void uart_send(int c)
{
        vaddr_t base = phys_to_virt(get_uart_base());

        serial8250_uart_send(base, c);
}

u32 uart_recv(void)
{
        return 0;
}

u32 nb_uart_recv(void)
{
        return 0;
}
