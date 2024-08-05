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

#include <object/thread.h>
#include <uapi/ptrace.h>

int sys_ptrace(int req, unsigned long cap, unsigned long tid,
		void *addr, void *data);

int arch_ptrace_getregs(struct thread *thread,
			struct chcore_user_regs_struct *user_regs);
int arch_ptrace_getreg(struct thread *thread, unsigned long addr,
		       unsigned long *val);
int arch_ptrace_setreg(struct thread *thread, unsigned long addr,
		       unsigned long data);
int arch_set_thread_singlestep(struct thread *thread, bool step);

void handle_breakpoint(void);
void handle_singlestep(void);

void ptrace_deinit(void *);
