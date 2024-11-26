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

#include "boot.h"
#include "image.h"
#include "consts.h"

#include <common/types.h>

char boot_cpu_stack[PLAT_CPU_NUMBER][INIT_STACK_SIZE] ALIGN(4096);

/*
 * Initialize these variables in order to make them not in .bss section.
 * So, they will have concrete initial value even on real machine.
 *
 * Non-primary CPUs will spin until they see the secondary_boot_flag becomes
 * non-zero which is set in kernel (see enable_smp_cores).
 *
 * The secondary_boot_flag is initialized as {NOT_BSS, 0, 0, ...}.
 */
#define NOT_BSS (0xBEEFUL)
long secondary_boot_flag[PLAT_CPU_NUMBER] = {NOT_BSS};
volatile u64 clear_bss_flag = NOT_BSS;

/* Uart */
void early_uart_init(void);
void uart_send_string(char *);

static void wakeup_other_cores(void)
{
	u64 *addr;

	/*
	 * Set the entry address for non-primary cores.
	 * 0xe0, 0xe8, 0xf0 are fixed in the firmware (armstub8.bin).
	 */
	// addr = (u64 *)0xd8;
	// *addr = TEXT_OFFSET;
	addr = (u64 *)0xe0;
	*addr = TEXT_OFFSET;
	addr = (u64 *)0xe8;
	*addr = TEXT_OFFSET;
	addr = (u64 *)0xf0;
	*addr = TEXT_OFFSET;

	/*
	 * Instruction sev (set event) for waking up other (non-primary) cores
	 * that executes wfe instruction.
	 */
	asm volatile("sev");
}

static void clear_bss(void)
{
	u64 bss_start_addr;
	u64 bss_end_addr;
	u64 i;

	bss_start_addr = (u64)&_bss_start;
	bss_end_addr = (u64)&_bss_end;

	for (i = bss_start_addr; i < bss_end_addr; ++i)
		*(char *)i = 0;

	clear_bss_flag = 0;
}

void init_c(void)
{
	/* Clear the bss area for the kernel image */
	clear_bss();

	/* Initialize UART before enabling MMU. */
	early_uart_init();
	uart_send_string("boot: init_c\r\n");

	wakeup_other_cores();

	/* Initialize Kernel Page Table. */
	uart_send_string("[BOOT] Install kernel page table\r\n");
	init_kernel_pt();

	/* Enable MMU. */
	el1_mmu_activate();
	uart_send_string("[BOOT] Enable el1 MMU\r\n");

	/* Call Kernel Main. */
	uart_send_string("[BOOT] Jump to kernel main\r\n");
	start_kernel(secondary_boot_flag);

	/* Never reach here */
}

void secondary_init_c(int cpuid)
{
	el1_mmu_activate();
	secondary_cpu_boot(cpuid);
}
