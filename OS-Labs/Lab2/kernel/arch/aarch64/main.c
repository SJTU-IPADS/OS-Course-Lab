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

#include <sched/sched.h>
#include <sched/fpu.h>
#include <common/kprint.h>
#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/lock.h>
#include <arch/boot.h>
#include <arch/machine/smp.h>
#include <arch/machine/pmu.h>
#include <arch/mm/page_table.h>
#include <arch/mmu.h>
#include <mm/mm.h>
#include <io/uart.h>
#include <machine.h>
#include <irq/irq.h>
#include <object/thread.h>

ALIGN(STACK_ALIGNMENT)
char cpu_stacks[PLAT_CPU_NUM][CPU_STACK_SIZE];
struct lock big_kernel_lock;

ALIGN(PAGE_SIZE)
char empty_page[4096] = {0};

__attribute__((section(".init.serial")))
ALIGN(PAGE_SIZE)
char serial_number[4096] = "0xDEADBEEF";

/* Kernel Test */
void run_test(void);

void init_fpu_owner_locks(void);

/*
 * @boot_flag is boot flag addresses for smp;
 * @info is now only used as board_revision for rpi4.
 */
void main(paddr_t boot_flag, void *info)
{
	u32 ret = 0;

	/* Init big kernel lock */
	ret = lock_init(&big_kernel_lock);
	kinfo("[ChCore] lock init finished\n");
	BUG_ON(ret != 0);

	/* Init uart: no need to init the uart again */
	uart_init();
	kinfo("[ChCore] uart init finished\n");

	/* Init per_cpu info */
	init_per_cpu_info(0);
	kinfo("[ChCore] per-CPU info init finished\n");

	/* Init mm */
	mm_init(info);

	kinfo("[ChCore] mm init finished\n");

	void lab2_test_buddy(void);
	lab2_test_buddy();
	void lab2_test_kmalloc(void);
	lab2_test_kmalloc();
	void lab2_test_page_table(void);
	lab2_test_page_table();
#if defined(CHCORE_KERNEL_PM_USAGE_TEST)
	void lab2_test_pm_usage(void);
	lab2_test_pm_usage();
#endif
	/* Mapping KSTACK into kernel page table. */
	map_range_in_pgtbl_kernel((void*)((unsigned long)boot_ttbr1_l0 + KBASE), 
			KSTACKx_ADDR(0),
			(unsigned long)(cpu_stacks[0]) - KBASE, 
			CPU_STACK_SIZE, VMR_READ | VMR_WRITE);

	/* Init exception vector */
	arch_interrupt_init();
	timer_init();
	kinfo("[ChCore] interrupt init finished\n");

	/* Enable PMU by setting PMCR_EL0 register */
	pmu_init();
	kinfo("[ChCore] pmu init finished\n");

	/* Init scheduler with specified policy */
#if defined(CHCORE_KERNEL_SCHED_PBFIFO)
	sched_init(&pbfifo);
#elif defined(CHCORE_KERNEL_RT)
	sched_init(&pbrr);
#else
	sched_init(&rr);
#endif
	kinfo("[ChCore] sched init finished\n");

	init_fpu_owner_locks();

	/* Other cores are busy looping on the boot_flag, wake up those cores */
	enable_smp_cores(boot_flag);
	kinfo("[ChCore] boot multicore finished\n");

#ifdef CHCORE_KERNEL_TEST
	kinfo("[ChCore] kernel tests start\n");
	run_test();
	kinfo("[ChCore] kernel tests done\n");
#endif /* CHCORE_KERNEL_TEST */

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	/* Create initial thread here, which use the `init.bin` */
	create_root_thread();
	kinfo("[ChCore] create initial thread done\n");
	kinfo("End of Kernel Checkpoints: %s\n", serial_number);

	/* Leave the scheduler to do its job */
	sched();

	/* Context switch to the picked thread */
	eret_to_thread(switch_context());

	/* Should provide panic and use here */
	BUG("[FATAL] Should never be here!\n");
}

void secondary_start(u32 cpuid)
{
	/* Init per_cpu info */
	init_per_cpu_info(cpuid);

	/* Mapping KSTACK into kernel page table. */
	map_range_in_pgtbl_kernel((void*)((unsigned long)boot_ttbr1_l0 + KBASE), 
			KSTACKx_ADDR(cpuid),
			(unsigned long)(cpu_stacks[cpuid]) - KBASE, 
			CPU_STACK_SIZE, VMR_READ | VMR_WRITE);

	arch_interrupt_init_per_cpu();

	/* Set the cpu status to inform the primary cpu */
	cpu_status[cpuid] = cpu_run;

	timer_init();
	pmu_init();

#ifdef CHCORE_KERNEL_TEST
	run_test();
#endif /* CHCORE_KERNEL_TEST */

#if FPU_SAVING_MODE == LAZY_FPU_MODE
	disable_fpu_usage();
#endif

	sched();
	eret_to_thread(switch_context());
}
