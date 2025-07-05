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

#ifdef CHCORE
#include <common/util.h>
#include <mm/kmalloc.h>
#endif

#include <common/vars.h>
#include <common/macro.h>
#include <common/types.h>
#include <common/errno.h>
#include <lib/printk.h>
#include <mm/vmspace.h>
#include <mm/mm.h>
#include <mm/common_pte.h>
#include <arch/mmu.h>
#include <arch/sync.h>

#include <arch/mm/page_table.h>

/* Page_table.c: Use simple impl for now. */

void set_page_table(paddr_t pgtbl)
{
        set_ttbr0_el1(pgtbl);
}

static int __vmr_prot_to_ap(vmr_prop_t prot)
{
        if ((prot & VMR_READ) && !(prot & VMR_WRITE)) {
                return AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO;
        } else if (prot & VMR_WRITE) {
                return AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW;
        }
        return 0;
}

static int __ap_to_vmr_prot(int ap)
{
        if (ap == AARCH64_MMU_ATTR_PAGE_AP_HIGH_RO_EL0_RO) {
                return VMR_READ;
        } else if (ap == AARCH64_MMU_ATTR_PAGE_AP_HIGH_RW_EL0_RW) {
                return VMR_READ | VMR_WRITE;
        }
        return 0;
}

#define USER_PTE   0
#define KERNEL_PTE 1

/*
 * the 3rd arg means the kind of PTE.
 */
__maybe_unused static int set_pte_flags(pte_t *entry, vmr_prop_t flags,
                                        int kind)
{
        BUG_ON(kind != USER_PTE && kind != KERNEL_PTE);

        /*
         * Current access permission (AP) setting:
         * Mapped pages are always readable (No considering XOM).
         * EL1 can directly access EL0 (No restriction like SMAP
         * as ChCore is a microkernel).
         */
        entry->l3_page.AP = __vmr_prot_to_ap(flags);

        // kernel PTE
        if (kind == KERNEL_PTE) {
                if (!(flags & VMR_EXEC))
                        entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
                entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;

        }
        // User PTE
        else {
                if (!(flags & VMR_EXEC))
                        entry->l3_page.UXN = AARCH64_MMU_ATTR_PAGE_UXN;
                // EL1 cannot directly execute EL0 accessible region.
                entry->l3_page.PXN = AARCH64_MMU_ATTR_PAGE_PXN;
        }

        // Set AF (access flag) in advance.
        entry->l3_page.AF = AARCH64_MMU_ATTR_PAGE_AF_ACCESSED;
        // Mark the mapping as not global
        entry->l3_page.nG = 1;
        // Mark the mappint as inner sharable
        entry->l3_page.SH = INNER_SHAREABLE;
        // Set the memory type
        if (flags & VMR_DEVICE) {
                entry->l3_page.attr_index = DEVICE_MEMORY;
                entry->l3_page.SH = 0;
        } else if (flags & VMR_NOCACHE) {
                entry->l3_page.attr_index = NORMAL_MEMORY_NOCACHE;
        } else {
                entry->l3_page.attr_index = NORMAL_MEMORY;
        }

        return 0;
}

#define GET_PADDR_IN_PTE(entry) \
        (((u64)(entry)->table.next_table_addr) << PAGE_SHIFT)
#define GET_NEXT_PTP(entry) phys_to_virt(GET_PADDR_IN_PTE(entry))

#define NORMAL_PTP (0)
#define BLOCK_PTP  (1)

/*
 * Find next page table page for the "va".
 *
 * cur_ptp: current page table page
 * level:   current ptp level
 *
 * next_ptp: returns "next_ptp"
 * pte     : returns "pte" (points to next_ptp) in "cur_ptp"
 *
 * alloc: if true, allocate a ptp when missing
 *
 */
static int get_next_ptp(ptp_t *cur_ptp, u32 level, vaddr_t va, ptp_t **next_ptp,
                        pte_t **pte, bool alloc, __maybe_unused long *rss)
{
        u32 index = 0;
        pte_t *entry;

        if (cur_ptp == NULL)
                return -ENOMAPPING;

        switch (level) {
        case L0:
                index = GET_L0_INDEX(va);
                break;
        case L1:
                index = GET_L1_INDEX(va);
                break;
        case L2:
                index = GET_L2_INDEX(va);
                break;
        case L3:
                index = GET_L3_INDEX(va);
                break;
        default:
                BUG("unexpected level\n");
                return -EINVAL;
        }

        entry = &(cur_ptp->ent[index]);
        if (IS_PTE_INVALID(entry->pte)) {
                if (alloc == false) {
                        return -ENOMAPPING;
                } else {
                        /* alloc a new page table page */
                        ptp_t *new_ptp;
                        paddr_t new_ptp_paddr;
                        pte_t new_pte_val;

                        /* alloc a single physical page as a new page table page
                         */
                        new_ptp = get_pages(0);
                        if (new_ptp == NULL)
                                return -ENOMEM;
                        memset((void *)new_ptp, 0, PAGE_SIZE);

                        new_ptp_paddr = virt_to_phys((vaddr_t)new_ptp);

                        new_pte_val.pte = 0;
                        new_pte_val.table.is_valid = 1;
                        new_pte_val.table.is_table = 1;
                        new_pte_val.table.next_table_addr = new_ptp_paddr
                                                            >> PAGE_SHIFT;

                        /* same effect as: cur_ptp->ent[index] = new_pte_val; */
                        entry->pte = new_pte_val.pte;
                }
        }

        *next_ptp = (ptp_t *)GET_NEXT_PTP(entry);
        *pte = entry;
        if (IS_PTE_TABLE(entry->pte))
                return NORMAL_PTP;
        else
                return BLOCK_PTP;
}

int debug_query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
        ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
        ptp_t *phys_page;
        pte_t *pte;
        int ret;

        // L0 page table
        l0_ptp = (ptp_t *)pgtbl;
        ret = get_next_ptp(l0_ptp, L0, va, &l1_ptp, &pte, false, NULL);
        if (ret < 0) {
                printk("[debug_query_in_pgtbl] L0 no mapping.\n");
                return ret;
        }
        printk("L0 pte is 0x%lx\n", pte->pte);

        // L1 page table
        ret = get_next_ptp(l1_ptp, L1, va, &l2_ptp, &pte, false, NULL);
        if (ret < 0) {
                printk("[debug_query_in_pgtbl] L1 no mapping.\n");
                return ret;
        }
        printk("L1 pte is 0x%lx\n", pte->pte);

        // L2 page table
        ret = get_next_ptp(l2_ptp, L2, va, &l3_ptp, &pte, false, NULL);
        if (ret < 0) {
                printk("[debug_query_in_pgtbl] L2 no mapping.\n");
                return ret;
        }
        printk("L2 pte is 0x%lx\n", pte->pte);

        // L3 page table
        ret = get_next_ptp(l3_ptp, L3, va, &phys_page, &pte, false, NULL);
        if (ret < 0) {
                printk("[debug_query_in_pgtbl] L3 no mapping.\n");
                return ret;
        }
        printk("L3 pte is 0x%lx\n", pte->pte);

        *pa = virt_to_phys((vaddr_t)phys_page) + GET_VA_OFFSET_L3(va);
        *entry = pte;
        return 0;
}

#ifdef CHCORE
void free_page_table(void *pgtbl)
{
        ptp_t *l0_ptp, *l1_ptp, *l2_ptp, *l3_ptp;
        pte_t *l0_pte, *l1_pte, *l2_pte;
        int i, j, k;

        if (pgtbl == NULL) {
                kwarn("%s: input arg is NULL.\n", __func__);
                return;
        }

        /* L0 page table */
        l0_ptp = (ptp_t *)pgtbl;

        /* Iterate each entry in the l0 page table*/
        for (i = 0; i < PTP_ENTRIES; ++i) {
                l0_pte = &l0_ptp->ent[i];
                if (IS_PTE_INVALID(l0_pte->pte))
                        continue;
                l1_ptp = (ptp_t *)GET_NEXT_PTP(l0_pte);

                /* Iterate each entry in the l1 page table*/
                for (j = 0; j < PTP_ENTRIES; ++j) {
                        l1_pte = &l1_ptp->ent[j];
                        if (IS_PTE_INVALID(l1_pte->pte))
                                continue;
                        l2_ptp = (ptp_t *)GET_NEXT_PTP(l1_pte);

                        /* Iterate each entry in the l2 page table*/
                        for (k = 0; k < PTP_ENTRIES; ++k) {
                                l2_pte = &l2_ptp->ent[k];
                                if (IS_PTE_INVALID(l2_pte->pte))
                                        continue;
                                l3_ptp = (ptp_t *)GET_NEXT_PTP(l2_pte);
                                /* Free the l3 page table page */
                                kfree(l3_ptp);
                        }

                        /* Free the l2 page table page */
                        kfree(l2_ptp);
                }

                /* Free the l1 page table page */
                kfree(l1_ptp);
        }

        kfree(l0_ptp);
}
#endif

/*
 * Translate a va to pa, and get its pte for the flags
 */
int query_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t *pa, pte_t **entry)
{
        /* LAB 2 TODO 4 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * return the pa and pte until a L2/L3 block or page, return
         * `-ENOMAPPING` if the va is not mapped.
         */
        /* BLANK BEGIN */

        /* BLANK END */
        /* LAB 2 TODO 4 END */
        return 0;
}

static int map_range_in_pgtbl_common(void *pgtbl, vaddr_t va, paddr_t pa,
                                     size_t len, vmr_prop_t flags, int kind,
                                     __maybe_unused long *rss)
{
        /* LAB 2 TODO 4 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * create new page table page if necessary, fill in the final level
         * pte with the help of `set_pte_flags`. Iterate until all pages are
         * mapped.
         * Since we are adding new mappings, there is no need to flush TLBs.
         * Return 0 on success.
         */
        /* BLANK BEGIN */

        /* BLANK END */
        /* LAB 2 TODO 4 END */
        dsb(ishst);
        isb();
        return 0;
}

/* Map vm range in kernel */
int map_range_in_pgtbl_kernel(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                              vmr_prop_t flags)
{
        return map_range_in_pgtbl_common(
                pgtbl, va, pa, len, flags, KERNEL_PTE, NULL);
}

/* Map vm range in user */
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                       vmr_prop_t flags, __maybe_unused long *rss)
{
        return map_range_in_pgtbl_common(
                pgtbl, va, pa, len, flags, USER_PTE, rss);
}

/*
 * Try to release a lower level page table page (low_ptp).
 * @high_ptp: the higher level page table page
 * @low_ptp: the next level page table page
 * @index: the index of low_ptp in high ptp entries
 * @return:
 * 	- zero if lower page table page is not all empty
 * 	- nonzero otherwise
 */
static int try_release_ptp(ptp_t *high_ptp, ptp_t *low_ptp, int index,
                           __maybe_unused long *rss)
{
        int i;

        for (i = 0; i < PTP_ENTRIES; i++) {
                if (low_ptp->ent[i].pte != PTE_DESCRIPTOR_INVALID) {
                        return 0;
                }
        }

        BUG_ON(index < 0 || index >= PTP_ENTRIES);
        high_ptp->ent[index].pte = PTE_DESCRIPTOR_INVALID;
        kfree(low_ptp);

        return 1;
}

__maybe_unused static void recycle_pgtable_entry(ptp_t *l0_ptp, ptp_t *l1_ptp,
                                                 ptp_t *l2_ptp, ptp_t *l3_ptp,
                                                 vaddr_t va,
                                                 __maybe_unused long *rss)
{
        if (!try_release_ptp(l2_ptp, l3_ptp, GET_L2_INDEX(va), rss))
                return;

        if (!try_release_ptp(l1_ptp, l2_ptp, GET_L1_INDEX(va), rss))
                return;

        try_release_ptp(l0_ptp, l1_ptp, GET_L0_INDEX(va), rss);
}

int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len,
                         __maybe_unused long *rss)
{
        /* LAB 2 TODO 4 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * mark the final level pte as invalid. Iterate until all pages are
         * unmapped.
         * You don't need to flush tlb here since tlb is now flushed after
         * this function is called.
         * Return 0 on success.
         */
        /* BLANK BEGIN */

        /* BLANK END */
        /* LAB 2 TODO 4 END */

        dsb(ishst);
        isb();

        return 0;
}

int mprotect_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, vmr_prop_t flags)
{
        /* LAB 2 TODO 4 BEGIN */
        /*
         * Hint: Walk through each level of page table using `get_next_ptp`,
         * modify the permission in the final level pte using `set_pte_flags`.
         * The `kind` argument of `set_pte_flags` should always be `USER_PTE`.
         * Return 0 on success.
         */
        /* BLANK BEGIN */

        /* BLANK END */
        /* LAB 2 TODO 4 END */
        return 0;
}

void parse_pte_to_common(pte_t *pte, unsigned int level,
                         struct common_pte_t *ret)
{
        switch (level) {
        case L3:
                ret->ppn = pte->l3_page.pfn;
                ret->perm = 0;
                ret->_unused = 0;
                ret->perm |= (pte->l3_page.UXN ? 0 : VMR_EXEC);
                ret->perm |= __ap_to_vmr_prot(pte->l3_page.AP);

                ret->perm |= (pte->l3_page.attr_index == DEVICE_MEMORY ?
                                      VMR_DEVICE :
                                      0);

                ret->access = pte->l3_page.AF;
                ret->dirty = pte->l3_page.DBM;
                ret->valid = pte->l3_page.is_valid;
                break;
        default:
                BUG("parse upper level PTEs is not supported now!\n");
        }
}

void update_pte(pte_t *dest, unsigned int level, struct common_pte_t *src)
{
        switch (level) {
        case L3:
                dest->l3_page.pfn = src->ppn;
                dest->l3_page.AP = __vmr_prot_to_ap(src->perm);

                dest->l3_page.UXN = ((src->perm & VMR_EXEC) ?
                                             AARCH64_MMU_ATTR_PAGE_UX :
                                             AARCH64_MMU_ATTR_PAGE_UXN);

                dest->l3_page.is_valid = src->valid;
#if !(defined(CHCORE_PLAT_RASPI3) || defined(CHCORE_PLAT_RASPI4) \
      || defined(CHCORE_PLAT_FT2000))
                /**
                 * Some platforms do not support setting AF and DBM
                 * by hardware, so on these platforms we ignored them.
                 */
                dest->l3_page.AF = src->access;
                dest->l3_page.DBM = src->dirty;
#endif
                break;
        default:
                BUG("update upper level PTEs is not supported now!\n");
        }
}
