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

#ifndef __ASM__
#include <machine.h>
#include <common/types.h>
#include <common/vars.h>

struct tss {
	u8 reserved0[4];
	u64 rsp[3];
	u64 ist[8];
	u8 reserved1[10];
	u16 iomba;
	u8 iopb[0];
} __attribute__ ((packed));
#endif

/*
 * The offset in the per_cpu struct, i.e., struct per_cpu_info.
 * The base addr of this struct is stored in GS register.
 *
 * IMPORTANT: modify the following offset values after
 * modifying struct per_cpu_info.
 */
#define OFFSET_CURRENT_SYSCALL_NO	0
#define OFFSET_CURRENT_EXEC_CTX		8
#define OFFSET_CURRENT_FPU_OWNER	16
#define OFFSET_FPU_DISABLE		24
#define OFFSET_LOCAL_CPU_ID		28
#define OFFSET_LOCAL_CPU_STACK		32
#define OFFSET_LOCAL_APIC_ID		40
#define OFFSET_CURRENT_TSS		44
/* offset for tss->rsp[0]: 44 + 4 */
#define OFFSET_CURRENT_INTERRUPT_STACK  48

#ifndef __ASM__
struct per_cpu_info {
	/* Used for handling syscall (saving rax, i.e., syscall_num) */
	u64 cur_syscall_no;
	/*
	 * Used for handling syscall to find the execution context of
	 * current thread.
	 */
	u64 cur_exec_ctx;

	/* struct thread *fpu_owner */
	void *fpu_owner;
	u32 fpu_disable;

	/* Local CPU ID */
	u32 cpu_id;

	/* Per-CPU stack */
	char *cpu_stack;

	/* APIC ID */
	u32 apic_id;

	/* per-CPU TSS area: for setting interrupt stack */
	struct tss tss;

	volatile u8 cpu_status;

	char pad[pad_to_cache_line(sizeof(u64) * 2 +
				   sizeof(void *) +
				   sizeof(u32) * 3 +
				   sizeof(char *) +
				   sizeof(struct tss) +
				   sizeof(u8))];
} __attribute__((packed, aligned(64)));

extern struct per_cpu_info cpu_info[PLAT_CPU_NUM];

enum cpu_state {
	cpu_hang = 0,
	cpu_run = 1,
	cpu_idle = 2
};

extern volatile char cpu_status[PLAT_CPU_NUM];

extern char cpu_stacks[PLAT_CPU_NUM][CPU_STACK_SIZE];

void enable_smp_cores(void);
u32 smp_get_cpu_id(void);
void init_per_cpu_info(u32 cpuid);
void flush_boot_tlb(void);

#endif
