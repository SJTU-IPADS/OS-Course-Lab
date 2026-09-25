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

#include <irq/ipi.h>
#include <sched/sched.h>

void arch_send_ipi(u32 cpu, u32 ipi)
{
	plat_send_ipi(cpu, ipi);
}

void arch_handle_ipi(u32 ipi_vector)
{
	switch (ipi_vector) {
	case IPI_RESCHED:
		add_pending_resched(smp_get_cpu_id());
		break;
	default:
		BUG("Unsupported IPI vector %u\n", ipi_vector);
		break;
	}
}
