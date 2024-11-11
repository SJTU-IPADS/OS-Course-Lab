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

#include <arch/machine/esr.h>
#include <arch/mmu.h>
#include <common/types.h>
#include <mm/extable.h>
#include <mm/page_fault.h>
#include <mm/vmspace.h>
#include <object/recycle.h>
#include <object/thread.h>

#include "irq_entry.h"

static inline vaddr_t get_fault_addr(void)
{
        vaddr_t addr;
        asm volatile("mrs %0, far_el1\n\t" : "=r"(addr));
        return addr;
}

static void __do_kernel_fault(u64 esr, u64 fault_ins_addr, u64 *fix_addr)
{
        kdebug("kernel_fault triggered\n");
        if (fixup_exception(fault_ins_addr, fix_addr)) {
                return;
        }

        BUG_ON(1);

        sys_exit_group(-1);
}

// EC: Instruction Abort or Data Abort
void do_page_fault(u64 esr, u64 fault_ins_addr, int type, u64 *fix_addr)
{
        vaddr_t fault_addr;
        int fsc; // fault status code
        int wnr;
        int ret;

        fault_addr = get_fault_addr();
        fsc = GET_ESR_EL1_FSC(esr);
        switch (fsc) {
        case DFSC_TRANS_FAULT_L0:
        case DFSC_TRANS_FAULT_L1:
        case DFSC_TRANS_FAULT_L2:
        case DFSC_TRANS_FAULT_L3: {

                ret = handle_trans_fault(current_thread->vmspace, fault_addr);
                if (ret != 0) {
                        /* The trap happens in the kernel */
                        if (type < SYNC_EL0_64) {
                                goto no_context;
                        }

                        kinfo("do_page_fault: faulting ip is 0x%lx (real IP),"
                              "faulting address is 0x%lx,"
                              "fsc is trans_fault (0b%b),"
                              "type is %d\n",
                              fault_ins_addr,
                              fault_addr,
                              fsc,
                              type);
                        kprint_vmr(current_thread->vmspace);

                        kinfo("current_cap_group is %s\n",
                                        current_cap_group->cap_group_name);

                        sys_exit_group(-1);
                }
                break;
        }
        case DFSC_PERM_FAULT_L1:
        case DFSC_PERM_FAULT_L2:
        case DFSC_PERM_FAULT_L3:
                wnr = GET_ESR_EL1_WnR(esr);
                if (wnr) {
                        ret = handle_perm_fault(
                                current_thread->vmspace, fault_addr, VMR_WRITE);
                } else {
                        ret = handle_perm_fault(
                                current_thread->vmspace, fault_addr, VMR_READ);
                }

                if (ret != 0) {
                        /* The trap happens in the kernel */
                        if (type < SYNC_EL0_64) {
                                goto no_context;
                        }
                        sys_exit_group(-1);
                }
                break;
        case DFSC_ACCESS_FAULT_L1:
        case DFSC_ACCESS_FAULT_L2:
        case DFSC_ACCESS_FAULT_L3:
                kinfo("do_page_fault: fsc is access_fault (0b%b)\n", fsc);
                BUG_ON(1);
                break;
        default:
                kinfo("do_page_fault: faulting ip is 0x%lx (real IP),"
                      "faulting address is 0x%lx,"
                      "fsc is unsupported now (0b%b)\n",
                      fault_ins_addr,
                      fault_addr,
                      fsc);
                kprint_vmr(current_thread->vmspace);

                kinfo("current_cap_group is %s\n",
                      current_cap_group->cap_group_name);

                BUG_ON(1);
                break;
        }

        return;

no_context:
        kinfo("kernel_fault: faulting ip is 0x%lx (real IP),"
              "faulting address is 0x%lx,"
              "fsc is 0b%b\n",
              fault_ins_addr,
              fault_addr,
              fsc);
        __do_kernel_fault(esr, fault_ins_addr, fix_addr);
}
