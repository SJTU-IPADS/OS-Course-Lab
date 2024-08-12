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

#include "uart.h"

void early_uart_init(void)
{
	unsigned int ra;

	ra = early_get32(GPFSEL1);
	ra &= ~(7 << 12);
	ra |= 2 << 12;
	ra &= ~(7 << 15);
	ra |= 2 << 15;
	early_put32(GPFSEL1, ra);

	early_put32(GPPUD, 0);
	delay(150);
	early_put32(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	early_put32(GPPUDCLK0, 0);

	early_put32(AUX_ENABLES, 1);
	early_put32(AUX_MU_IER_REG, 0);
	early_put32(AUX_MU_CNTL_REG, 0);
	early_put32(AUX_MU_IER_REG, 0);
	early_put32(AUX_MU_LCR_REG, 3);
	early_put32(AUX_MU_MCR_REG, 0);
	early_put32(AUX_MU_BAUD_REG, 270);

	early_put32(AUX_MU_CNTL_REG, 3);
}

unsigned int early_uart_lsr(void)
{
	return early_get32(AUX_MU_LSR_REG);
}

static void early_uart_send(unsigned int c)
{
	while (1) {
		if (early_uart_lsr() & 0x20)
			break;
	}
	early_put32(AUX_MU_IO_REG, c);
}

void uart_send_string(char *str)
{
	int i;
	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] == '\n')
			early_uart_send('\r');
		early_uart_send(str[i]);
	}
}
