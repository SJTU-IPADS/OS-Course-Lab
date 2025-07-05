/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#include <arch/mm/page_table.h>
#include <mm/cache.h>
#include <common/backtrace.h>
#include <common/errno.h>
#include <common/util.h>
#include <mm/mm.h>
#include <mm/vmspace.h>
#include <mm/common_pte.h>
#include <sched/context.h>
#include <object/recycle.h>
#include <object/user_fault.h>
#include <object/thread.h>
#include <mm/page_fault.h>

static void dump_pgfault_error(void)
{
        kinfo("kernel dump:\n");
        kinfo("process: %p\n", current_cap_group);
        print_thread(current_thread);
        kinfo("faulting IP: 0x%lx, SP: 0x%lx\n",
              arch_get_thread_next_ip(current_thread),
              arch_get_thread_stack(current_thread));
        kprint_vmr(current_thread->vmspace);

        backtrace();
}

/*
 * Perform general COW
 * Step-1: get PA of page containing fault_addr, so as kernal VA of that page
 * Step-2: allocate a new page and record in VMR
 * Step-3: copy using kernel VA to new page
 * Step-4(?): update VMR perm (How and when? Necessary?)
 * Step-5: update PTE permission and PPN
 * Step-6: Flush TLB of user virtual page(user_vpa)
 */
static int __do_general_cow(struct vmspace *vmspace, struct vmregion *vmr,
                            vaddr_t fault_addr, pte_t *fault_pte,
                            struct common_pte_t *pte_info)
{
        vaddr_t kva, user_vpa;
        void *new_page;
        paddr_t new_pa;
        struct common_pte_t new_pte_attr;
        int ret = 0;

        /* Step-1: get PA of page containing fault_addr, so as kernal VA of that
         * page */
        kva = phys_to_virt(pte_info->ppn << PAGE_SHIFT);

        /* Step-2: allocate a new page and record in VMR */
        new_page = get_pages(0);
        if (!new_page) {
                ret = -ENOMEM;
                goto out;
        }
        new_pa = virt_to_phys(new_page);

        ret = vmregion_record_cow_private_page(vmr, fault_addr, new_page);
        if (ret)
                goto out_free_page;

        /* Step-3: copy using kernel VA to new page */
        memcpy(new_page, (void *)kva, PAGE_SIZE);

        /* Step-5: update PTE permission and PPN */
        new_pte_attr.ppn = new_pa >> PAGE_SHIFT;
        new_pte_attr.perm = pte_info->perm | VMR_WRITE;
        new_pte_attr.valid = 1;
        new_pte_attr.access = 0;
        new_pte_attr.dirty = 0;

        update_pte(fault_pte, L3, &new_pte_attr);

        /* Step-6: Flush TLB of user virtual page(user_vpa) */
        user_vpa = ROUND_DOWN(fault_addr, PAGE_SIZE);
        flush_tlb_by_range(vmspace, user_vpa, PAGE_SIZE);

        return 0;
out_free_page:
        free_pages(new_page);
out:
        return ret;
}

static int do_cow(struct vmspace *vmspace, struct vmregion *fault_vmr,
                  vaddr_t fault_addr)
{
        int ret = 0;
        paddr_t pa;
        pte_t *fault_pte;
        struct common_pte_t pte_info;
        lock(&vmspace->pgtbl_lock);
        ret = query_in_pgtbl(vmspace->pgtbl, fault_addr, &pa, &fault_pte);
        /**
         * Although we are handling a permission fault here, it's still possible
         * that we would discover it should be a translation error when we
         * really started to resolve it here. For example, before the page fault
         * was forwarded to here, we didn't grab the pgtbl_lock, so it's
         * possible that the page was unmapped or swapped out. There is another
         * special case: in RISC-V, we are unable to determine a page fault is
         * translation fault or permission fault from scause reported by the
         * hardware, so we have to check the pte here.
         *
         * We query the page table for the actual PTE **atomically** here. So if
         * the PTE is missing, we can be sure that it's a translation fault. And
         * we forward it back to translation fault handler by returning -EFAULT.
         */
        if (ret) {
                ret = -EFAULT;
                goto out;
        }
        parse_pte_to_common(fault_pte, L3, &pte_info);

        // Fast path: already handled page fault in other threads
        if (pte_info.perm & VMR_WRITE) {
                goto out;
        }
        ret = __do_general_cow(
                vmspace, fault_vmr, fault_addr, fault_pte, &pte_info);

out:
        unlock(&vmspace->pgtbl_lock);
        return ret;
}

static int check_trans_fault(struct vmspace *vmspace, vaddr_t fault_addr)
{
        int ret = 0;
        paddr_t pa;
        pte_t *fault_pte;

        lock(&vmspace->pgtbl_lock);
        ret = query_in_pgtbl(vmspace->pgtbl, fault_addr, &pa, &fault_pte);
        unlock(&vmspace->pgtbl_lock);
        if (ret) {
                return -EFAULT;
        }

        return 0;
}

int handle_trans_fault(struct vmspace *vmspace, vaddr_t fault_addr)
{
        struct vmregion *vmr;
        struct pmobject *pmo;
        paddr_t pa;
        unsigned long offset;
        unsigned long index;
        int ret = 0;

        /*
         * Grab lock here.
         * Because two threads (in same process) on different cores
         * may fault on the same page, so we need to prevent them
         * from adding the same mapping twice.
         */
        lock(&vmspace->vmspace_lock);
        vmr = find_vmr_for_va(vmspace, fault_addr);

        if (vmr == NULL) {
                kinfo("handle_trans_fault: no vmr found for va 0x%lx!\n",
                      fault_addr);
                dump_pgfault_error();
                unlock(&vmspace->vmspace_lock);

#if defined(CHCORE_ARCH_AARCH64) || defined(CHCORE_ARCH_SPARC)
                /* kernel fault fixup is only supported on AArch64 and Sparc */
                return -EFAULT;
#endif
                sys_exit_group(-1);

                BUG("should not reach here");
        }

        pmo = vmr->pmo;
        /* Get the offset in the pmo for faulting addr */
        offset = ROUND_DOWN(fault_addr, PAGE_SIZE) - vmr->start + vmr->offset;
        vmr_prop_t perm = vmr->perm;
        switch (pmo->type) {
        case PMO_ANONYM:
        case PMO_SHM: {
                /* Boundary check */
                BUG_ON(offset >= pmo->size);

                /* Get the index in the pmo radix for faulting addr */
                index = offset / PAGE_SIZE;

                fault_addr = ROUND_DOWN(fault_addr, PAGE_SIZE);

                pa = get_page_from_pmo(pmo, index);
                if (pa == 0) {
                        /*
                         * Not committed before. Then, allocate the physical
                         * page.
                         */
                        /* LAB 2 TODO 7 BEGIN */
                        /* BLANK BEGIN */
                        /* Hint: Allocate a physical page and clear it to 0. */

                        /* BLANK END */
                        /*
                         * Record the physical page in the radix tree:
                         * the offset is used as index in the radix tree
                         */
                        kdebug("commit: index: %ld, 0x%lx\n", index, pa);
                        commit_page_to_pmo(pmo, index, pa);

                        /* Add mapping in the page table */
                        lock(&vmspace->pgtbl_lock);
                        /* BLANK BEGIN */

                        /* BLANK END */
                        unlock(&vmspace->pgtbl_lock);
                } else {
                        /*
                         * pa != 0: the faulting address has be committed a
                         * physical page.
                         *
                         * For concurrent page faults:
                         *
                         * When type is PMO_ANONYM, the later faulting threads
                         * of the process do not need to modify the page
                         * table because a previous faulting thread will do
                         * that. (This is always true for the same process)
                         * However, if one process map an anonymous pmo for
                         * another process (e.g., main stack pmo), the faulting
                         * thread (e.g, in the new process) needs to update its
                         * page table.
                         * So, for simplicity, we just update the page table.
                         * Note that adding the same mapping is harmless.
                         *
                         * When type is PMO_SHM, the later faulting threads
                         * needs to add the mapping in the page table.
                         * Repeated mapping operations are harmless.
                         */
                        if (pmo->type == PMO_SHM || pmo->type == PMO_ANONYM) {
                                /* Add mapping in the page table */
                                lock(&vmspace->pgtbl_lock);
                                /* BLANK BEGIN */

                                /* BLANK END */
                                /* LAB 2 TODO 7 END */
                                unlock(&vmspace->pgtbl_lock);
                        }
                }

                if (perm & VMR_EXEC) {
                        arch_flush_cache(fault_addr, PAGE_SIZE, SYNC_IDCACHE);
                }

                break;
        }
        case PMO_FILE: {
                unlock(&vmspace->vmspace_lock);
                fault_addr = ROUND_DOWN(fault_addr, PAGE_SIZE);
                handle_user_fault(pmo, ROUND_DOWN(fault_addr, PAGE_SIZE));
                BUG("Should never be here!\n");
                break;
        }
        case PMO_FORBID: {
                kinfo("Forbidden memory access (pmo->type is PMO_FORBID).\n");
                dump_pgfault_error();

                unlock(&vmspace->vmspace_lock);
                sys_exit_group(-1);

                BUG("should not reach here");
                break;
        }
        default: {
                kinfo("handle_trans_fault: faulting vmr->pmo->type"
                      "(pmo type %d at 0x%lx)\n",
                      vmr->pmo->type,
                      fault_addr);
                dump_pgfault_error();

                unlock(&vmspace->vmspace_lock);
                sys_exit_group(-1);

                BUG("should not reach here");
                break;
        }
        }

        unlock(&vmspace->vmspace_lock);
        return ret;
}

// clang-format off
/**
 * @brief Handle a permission fault triggered by hardware. This function
 * is arch-independent.
 *
 * A permission fault is that the permission required to execute an
 * instruction cannot be satisfied by permission of a page in page table
 * when **executing** the instruction by the hardware. To handle a
 * permission fault, we have to differentiate permissions in the VMR where
 * the faulting page belongs(declared_perm) and the privilege needed
 * by the faulting instruction(desired_perm). In some cases, a permission
 * fault is demanded by us. For instance, if a page is shared through CoW,
 * it's mapped as readonly sharing at first, and there would be a permission
 * fault when it's written, and we could do the copy part of CoW in permission
 * fault handlers. In other cases, a permission fault might indicate a bug or
 * a malicious attack, and we would kill the faulting process.
 *
 * It's worth noting that there is a time window between the processor core
 * try to execute the faulting instruction and we started to handle the perm
 * fault here. But the faulting process and our kernel are multithreaded.
 * During the time window, the faulting page may has been unmapped by other
 * threads of the process, or swapped out by the kernel. So, we could figure
 * out that we find nothing in page table when we query the faulting address
 * surprisingly. Under such circumstances, we would treat this permission fault
 * as a translation fault and handle it for correctness.
 *
 * In conclusion, following execution flow is possible when handling a
 * permission fault. Noting that a permission fault might downgrade to a
 * translation fault, but it's impossible to get a reverse scenario.
 *
 * demanded fault: a permission fault with clearly defined desired_perm and
 * declared_perm combinations. e.g., declared = READ and desired = WRITE,
 * it would be forwarded to a specific handler, e.g. do_cow
 * 
 * A. demanded fault->specific handler->success 
 *      A.1. re-execute faulting instruction->success 
 *      A.2. page swapped out by another thread->re-execute->trans fault
 * B. demanded fault->specific handler->return an EFAULT->fallback to trans fault
 * C. demanded fault->specific handler->return other error->kill process
 * D. undemanded fault->check is trans fault->fallback to trans fault
 * E. undemanded fault->check is not trans fault->kill process
 */
// clang-format on
int handle_perm_fault(struct vmspace *vmspace, vaddr_t fault_addr,
                      vmr_prop_t desired_perm)
{
        int ret = 0;
        struct vmregion *vmr;
        vmr_prop_t declared_perm;

        lock(&vmspace->vmspace_lock);
        vmr = find_vmr_for_va(vmspace, fault_addr);

        if (vmr == NULL) {
                kinfo("handle_perm_fault: no vmr found for va 0x%lx!\n",
                      fault_addr);
                dump_pgfault_error();
                unlock(&vmspace->vmspace_lock);
#if defined(CHCORE_ARCH_AARCH64) || defined(CHCORE_ARCH_SPARC)
                return -EFAULT;
#else
                sys_exit_group(-1);
                BUG("should not reach here");
#endif
        }

        declared_perm = vmr->perm;

        // Handle write to a read-only page
        if ((declared_perm & VMR_READ) && desired_perm == VMR_WRITE) {
                // Handle COW here
                if (declared_perm & VMR_COW) {
                        ret = do_cow(vmspace, vmr, fault_addr);
                        if (ret != 0 && ret != -EFAULT) {
                                goto out_illegal;
                        } else if (ret == -EFAULT) {
                                goto out_trans_fault;
                        } else {
                                goto out_succ;
                        }
                }
        }

        /**
         * For other invalid declared/desired permission combinations,
         * we check whether it's a trans fault actually here, if so we
         * handle it. But if not, we could be sure that it's an access
         * try with illegal permission, so we kill the faulting process.
         */
        ret = check_trans_fault(vmspace, fault_addr);
        if (ret == -EFAULT) {
                goto out_trans_fault;
        } else {
                goto out_illegal;
        }
out_succ:
        unlock(&vmspace->vmspace_lock);
        return ret;
out_illegal:
        // Illegal access permission, kill process
        kinfo("handle_perm_fault failed: fault_addr=%p desired_perm=%lu\n",
              fault_addr,
              desired_perm);
        dump_pgfault_error();
        unlock(&vmspace->vmspace_lock);
#if defined(CHCORE_ARCH_AARCH64) || defined(CHCORE_ARCH_SPARC)
        return -EPERM;
#else
        sys_exit_group(-1);
        return -1;
#endif
out_trans_fault:
        /**
         * It's possible for a permission fault to downgrade to a translation
         * fault. For example, the page has been swapped out or unmapped
         * concurrently before the permission fault is forwarded to here. We let
         * actual permission fault handlers(e.g. do_cow) to return -EFAULT when
         * they detect such cases, and treat it as a translation fault then
         * handle it atomically.
         */
        unlock(&vmspace->vmspace_lock);
        ret = handle_trans_fault(vmspace, fault_addr);
        return ret;
}
