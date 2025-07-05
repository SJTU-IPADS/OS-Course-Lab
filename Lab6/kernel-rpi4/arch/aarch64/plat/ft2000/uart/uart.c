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
#include <arch/tools.h>
#include <io/uart.h>

#define UART_DR		0x00	/* data register */
#define UART_RSR_ECR	0x04	/* receive status or error clear */
#define UART_DMAWM	0x08	/* DMA watermark configure */
#define UART_TIMEOUT	0x0C	/* Timeout period */
/* reserved space */
#define UART_FR		0x18	/* flag register */
#define UART_ILPR	0x20	/* IrDA low-poer */
#define UART_IBRD	0x24	/* integer baud register */
#define UART_FBRD	0x28	/* fractional baud register */
#define UART_LCR_H	0x2C	/* line control register */
#define UART_CR		0x30	/* control register */
#define UART_IFLS	0x34	/* interrupt FIFO level select */
#define UART_IMSC	0x38	/* interrupt mask set/clear */
#define UART_RIS	0x3C	/* raw interrupt register */
#define UART_MIS	0x40	/* masked interrupt register */
#define UART_ICR	0x44	/* interrupt clear register */
#define UART_DMACR	0x48	/* DMA control register */

/* flag register bits */
#define UART_FR_RTXDIS	(1 << 13)
#define UART_FR_TERI	(1 << 12)
#define UART_FR_DDCD	(1 << 11)
#define UART_FR_DDSR	(1 << 10)
#define UART_FR_DCTS	(1 << 9)
#define UART_FR_RI	(1 << 8)
#define UART_FR_TXFE	(1 << 7)
#define UART_FR_RXFF	(1 << 6)
#define UART_FR_TXFF	(1 << 5)
#define UART_FR_RXFE	(1 << 4)
#define UART_FR_BUSY	(1 << 3)
#define UART_FR_DCD	(1 << 2)
#define UART_FR_DSR	(1 << 1)
#define UART_FR_CTS	(1 << 0)

/* transmit/receive line register bits */
#define UART_LCRH_SPS		(1 << 7)
#define UART_LCRH_WLEN_8	(3 << 5)
#define UART_LCRH_WLEN_7	(2 << 5)
#define UART_LCRH_WLEN_6	(1 << 5)
#define UART_LCRH_WLEN_5	(0 << 5)
#define UART_LCRH_FEN		(1 << 4)
#define UART_LCRH_STP2		(1 << 3)
#define UART_LCRH_EPS		(1 << 2)
#define UART_LCRH_PEN		(1 << 1)
#define UART_LCRH_BRK		(1 << 0)

/* control register bits */
#define UART_CR_CTSEN		(1 << 15)
#define UART_CR_RTSEN		(1 << 14)
#define UART_CR_OUT2		(1 << 13)
#define UART_CR_OUT1		(1 << 12)
#define UART_CR_RTS		(1 << 11)
#define UART_CR_DTR		(1 << 10)
#define UART_CR_RXE		(1 << 9)
#define UART_CR_TXE		(1 << 8)
#define UART_CR_LPE		(1 << 7)
#define UART_CR_OVSFACT		(1 << 3)
#define UART_CR_UARTEN		(1 << 0)

#define UART_IMSC_RTIM		(1 << 6)
#define UART_IMSC_RXIM		(1 << 4)

#define UART_BASE (KBASE + FT_UART1_BASE)

char uart_recv(void)
{
	/* Wait until there is space in the FIFO or device is disabled */
	while (1) {
		if (!(get32((unsigned long) UART_BASE + UART_FR)
		& UART_FR_RXFE))
			break;
	}
	return (get32((unsigned long) UART_BASE + UART_DR) & 0xFF);
}

char nb_uart_recv(void)
{
	if (!(get32((unsigned long) UART_BASE + UART_FR)
		& UART_FR_RXFE))
		return (get32((unsigned long) UART_BASE + UART_DR) & 0xFF);
	else
		return NB_UART_NRET;
}

void uart_send(char ch)
{
	/* Wait until there is space in the FIFO or device is disabled */
	while (get32((unsigned long) UART_BASE + UART_FR)
	       & UART_FR_TXFF) {
	}
	/* Send the character */
	put32((unsigned long) UART_BASE + UART_DR, (unsigned int)ch);
}

void uart_init(void)
{
	unsigned long base = UART_BASE;

	/* Clear all errors */
	put32(base + UART_RSR_ECR, 0);
	/* Disable everything */
	put32(base + UART_CR, 0);

	if (CONSOLE_BAUDRATE) {
		unsigned int divisor =
		    (CONSOLE_UART_CLK_IN_HZ ) / (16*CONSOLE_BAUDRATE);
		put32(base + UART_IBRD, divisor);
		put32(base + UART_FBRD, divisor>>8);
	}

	/* Configure TX to 8 bits, FIFO enabled (buffer up to 16 bytes). */
	put32(base + UART_LCR_H, UART_LCRH_WLEN_8 | UART_LCRH_FEN);

	/* Enable interrupts for receive and receive timeout */
	put32(base + UART_IMSC, UART_IMSC_RXIM | UART_IMSC_RTIM);
	/* Disable interrupts */
	// put32(base + UART_IMSC, 0);

	/* Enable UART and RX/TX */
	put32(base + UART_CR, UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);

	/* Flush the screen */
	uart_send(12);
	uart_send(27);
	uart_send('[');
	uart_send('2');
	uart_send('J');
}
