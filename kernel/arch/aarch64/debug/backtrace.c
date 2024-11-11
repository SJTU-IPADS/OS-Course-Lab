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

#include <common/types.h>
#include <common/errno.h>
#include <common/debug.h>
#include <common/macro.h>
#include <common/kprint.h>
#include <object/thread.h>
#include <sched/sched.h>
#include <mm/uaccess.h>
#include <mm/kmalloc.h>
#include <mm/vmspace.h>
#include <arch/mmu.h>
#include <common/backtrace.h>
#include <sched/context.h>

#if ENABLE_BACKTRACE_FUNC == ON
static inline __attribute__ ((always_inline))
u64 read_fp(void)
{
	u64 fp;
	__asm __volatile("mov %0, x29":"=r"(fp));
	return fp;
}

int set_backtrace_data(void *pc_buf, void *fp_buf, void *ip)
{
	u64 kbuf[2];
	u64 lr = 0;
	u64 fp = read_fp();
	int iskernel = true;
	int count = 1;
	int ret;
	paddr_t pa;
	pte_t *pte;

	*(u64 *)ip = arch_get_thread_next_ip(current_thread);

	while (1) {
		if (iskernel) {
			lr = ((u64 *) fp)[1];
			fp = ((u64 *) fp)[0];
		} else {
			/*
			 * Copy_from_user handles invalid access.
			 * But the function may be called with vmspace_lock
			 * held. The page fault triggered by copy_from_user
			 * may cause deadlock
			 */
			ret = query_in_pgtbl(current_thread->vmspace->pgtbl, fp, &pa, &pte);
			if (ret != 0) {
				goto out;
			}
			copy_from_user(kbuf, (void *)fp, 2 * sizeof(u64));
			lr = ((u64 *) kbuf)[1];
			fp = ((u64 *) kbuf)[0];
		}

		if (count >= BACKTRACE_MAX_COUNT) {
			goto out;
		}

		if (iskernel && fp < KBASE) {
			iskernel = false;
		}

		((u64 *)pc_buf)[count - 1] = lr;
		((u64 *)fp_buf)[count - 1] = fp;
		count++;
	}

 out:
	return count - 1;
}

int backtrace(void)
{
	u64 kbuf[2];
	u64 lr = 0;
	u64 fp = read_fp();
	int iskernel = true;
	int count = 1;
	int ret;
	paddr_t pa;
	pte_t *pte;

	print_thread(current_thread);

	kinfo("kernel:\n");

	while (1) {
		if (iskernel) {
			lr = ((u64 *) fp)[1];
			fp = ((u64 *) fp)[0];
		} else {
			/*
			 * Copy_from_user handles invalid access.
			 * But the function may be called with vmspace_lock
			 * held. The page fault triggered by copy_from_user
			 * may cause deadlock
			 */
			ret = query_in_pgtbl(current_thread->vmspace->pgtbl, fp, &pa, &pte);
			if (ret != 0) {
				goto out;
			}
			copy_from_user(kbuf, (void *)fp, 2 * sizeof(u64));
			lr = ((u64 *) kbuf)[1];
			fp = ((u64 *) kbuf)[0];
		}

		if (count >= BACKTRACE_MAX_COUNT)
			goto out;

		if (iskernel && fp < KBASE) {
			kinfo("user:\n");
			iskernel = false;
		}

		kinfo("\tbacktrace:%d, fp = %lx, pc = %lx\n", count, fp, lr);
		count++;
	}

 out:
	return count;
}
#else
int set_backtrace_data(void *pc_buf, void *fp_buf, void *ip)
{
        return 0;
}

int backtrace(void)
{
        return 0;
}
#endif
