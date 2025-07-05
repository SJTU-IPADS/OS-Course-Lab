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

/*
   MIT License

   Copyright (c) 2018 Sergey Matyukevich

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/*
 * ChCore refers to
 * https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/rpi-os.md
 * for the min-uart init process.
 */
#include <arch/tools.h>
#include <io/uart.h>
#include <irq/irq.h>
#include "peripherals.h"

static inline void delay(unsigned int cnt)
{
	while (cnt != 0)
		cnt--;
}

#if USE_mini_uart == 1

void uart_init(void)
{
	unsigned int ra;

	ra = get32(GPFSEL1);
	ra &= ~(7 << 12);
	ra |= 2 << 12;
	ra &= ~(7 << 15);
	ra |= 2 << 15;
	put32(GPFSEL1, ra);

	put32(GPPUD, 0);
	delay(150);
	put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	put32(GPPUDCLK0, 0);

	put32(AUX_ENABLES, 1);
	put32(AUX_MU_IER_REG, 0);
	put32(AUX_MU_CNTL_REG, 0);
	put32(AUX_MU_IER_REG, 0);
	put32(AUX_MU_LCR_REG, 3);
	put32(AUX_MU_MCR_REG, 0);
	put32(AUX_MU_BAUD_REG, 270);

	put32(AUX_MU_CNTL_REG, 3);

	/* Clear the screen */
	uart_send(12);
	uart_send(27);
	uart_send('[');
	uart_send('2');
	uart_send('J');
}

unsigned int uart_lsr(void)
{
	return get32(AUX_MU_LSR_REG);
}

char uart_recv(void)
{
	while (1) {
		if (uart_lsr() & 0x01)
			break;
	}

	return (char)(get32(AUX_MU_IO_REG) & 0xFF);
}

char nb_uart_recv(void)
{
	if(uart_lsr() & 0x01)
		return (char)((get32(AUX_MU_IO_REG) & 0xFF));
	else
		return NB_UART_NRET;
}

void uart_send(char c)
{
	while (1) {
		if (uart_lsr() & 0x20)
			break;
	}
	put32(AUX_MU_IO_REG, (unsigned int)c);
}

#else

#define UART_BUF_LEN 16
typedef struct uart_buffer {
	char buffer[UART_BUF_LEN];
	int read_pos;
	int put_pos;
} uart_buffer_t;

uart_buffer_t pl011_buffer;

void enable_uart_irq(int irq_num)
{
	/* Clear pending bits */
	put32(RASPI3_PL011_ICR, 0x7ff);
	/* Set interrupt mask: only enable the receive interrupt. */
	put32(RASPI3_PL011_IMSC, 1 << 4);
	// put32(RASPI3_PL011_IFLS, 0);

	pl011_buffer.read_pos = 0;
	pl011_buffer.put_pos = 0;

	// plat_enable_irqno(UART_IRQ);
	plat_enable_irqno(irq_num);
}

void uart_irq_handler(void)
{
	char input;

	/* Check if the recv fifo is empty. */
	// BUG_ON(get32(RASPI3_PL011_FR) & (1 << 4));

	input = get32(RASPI3_PL011_DR) & 0xFF;
	/*
	 * Clear the interrupt pending bit.
	 * Nevertheless, the above reading DR operation will clear the pending bit.
	 */
	// put32(RASPI3_PL011_ICR, 0x7ff);

	pl011_buffer.buffer[pl011_buffer.put_pos] = input;

	/*
	 * The internal fifo buffer is a ring buffer.
	 * Here is updating the put_pos.
	 */
	pl011_buffer.put_pos += 1;
	if (pl011_buffer.put_pos == UART_BUF_LEN)
		pl011_buffer.put_pos = 0;

	if (pl011_buffer.read_pos == pl011_buffer.put_pos) {
		pl011_buffer.read_pos += 1;
		if (pl011_buffer.read_pos == UART_BUF_LEN)
			pl011_buffer.read_pos = 0;
	}
}

void uart_init(void)
{
	unsigned int ra;

	ra = get32(GPFSEL1);

	/* Set GPIO14 as function 0. */
	ra &= ~(7 << 12);
	ra |= 4 << 12;
	/* Set GPIO15 as function 0. */
	ra &= ~(7 << 15);
	ra |= 4 << 15;

	put32(GPFSEL1, ra);
	put32(GPPUD, 0);
	delay(150);
	put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	put32(GPPUDCLK0, 0);

	/* Close serial briefly. */
	put32(RASPI3_PL011_CR, 0);
	/* Set baud rate as 115200. */
	put32(RASPI3_PL011_IBRD, 26);
	put32(RASPI3_PL011_FBRD, 3);
	/* Enable FIFO. */
	//put32(RASPI3_PL011_LCRH, (1 << 4) | (3 << 5));
	/* Disable FIFO. */
	put32(RASPI3_PL011_LCRH, (0 << 4) | (3 << 5));
	/* Inhibit interrupt. */
	put32(RASPI3_PL011_IMSC, 0);
	/* Enable serial to send or receive data. */
	put32(RASPI3_PL011_CR, 1 | (1 << 8) | (1 << 9));

	/* Clear the screen */
	uart_send(12);
	uart_send(27);
	uart_send('[');
	uart_send('2');
	uart_send('J');
}

unsigned int uart_fr(void)
{
	return get32(RASPI3_PL011_FR);
}

char uart_recv(void)
{
	/* Check if the fifo is empty. */
	while (uart_fr() & (1 << 4));

	return (char)(get32(RASPI3_PL011_DR) & 0xFF);
}

char nb_uart_recv(void)
{
	char ch;

	if (pl011_buffer.read_pos == pl011_buffer.put_pos)
		return NB_UART_NRET;

	ch = pl011_buffer.buffer[pl011_buffer.read_pos];

	pl011_buffer.read_pos += 1;
	if (pl011_buffer.read_pos == UART_BUF_LEN)
		pl011_buffer.read_pos = 0;

	return ch;

#if 0
	/*
	 * Check if the recv fifo is empty.
	 * If it is not empyt, receive the data.
	 * Otherwise return NB_UART_NRET.
	 */
	if(!(uart_fr() & (1 << 4)))
		return (get32(RASPI3_PL011_DR) & 0xFF);
	else
		return NB_UART_NRET;
#endif
}

void uart_send(char c)
{
	/* Check if the send fifo is full. */
	while (uart_fr() & (1 << 5)) ;
	put32(RASPI3_PL011_DR, (unsigned int)c);
}

#endif
