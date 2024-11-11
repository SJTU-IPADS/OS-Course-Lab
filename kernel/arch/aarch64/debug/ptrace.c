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

#include <arch/machine/registers.h>
#include <common/kprint.h>
#include <common/util.h>
#include <object/thread.h>

int arch_ptrace_getregs(struct thread *thread,
                        struct chcore_user_regs_struct *user_regs)
{
        kwarn("Ptrace is not supported on AArch64 yet.\n");
        return -EINVAL;
}

int arch_ptrace_getreg(struct thread *thread, unsigned long addr,
                       unsigned long *val)
{
        kwarn("Ptrace is not supported on AArch64 yet.\n");
        return -EINVAL;
}

int arch_ptrace_setreg(struct thread *thread, unsigned long addr,
                       unsigned long data)
{
        kwarn("Ptrace is not supported on AArch64 yet.\n");
        return -EINVAL;
}

int arch_set_thread_singlestep(struct thread *thread, bool step)
{
        kwarn("Ptrace is not supported on AArch64 yet.\n");
        return -EINVAL;
}