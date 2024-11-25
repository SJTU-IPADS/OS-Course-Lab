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

#include <common/types.h>
#include <common/list.h>
#include <common/errno.h>
#include <mm/vmspace.h>
#include <mm/kmalloc.h>
#include <mm/mm.h>
#include <mm/uaccess.h>
#include <arch/mmu.h>

struct cow_private_page {
        struct list_head node;
        vaddr_t vaddr;
        void *page;
};

static struct vmregion *alloc_vmregion(vaddr_t start, size_t len, size_t offset,
                                       vmr_prop_t perm, struct pmobject *pmo)
{
        struct vmregion *vmr;

        vmr = kmalloc(sizeof(*vmr));
        if (vmr == NULL)
                return NULL;

        vmr->start = start;
        vmr->size = len;
        vmr->offset = offset;
        vmr->perm = perm;
        vmr->pmo = pmo;
        list_add(&vmr->mapping_list_node, &pmo->mapping_list);

        if (pmo->type == PMO_DEVICE)
                vmr->perm |= VMR_DEVICE;
        else if (pmo->type == PMO_DATA_NOCACHE)
                vmr->perm |= VMR_NOCACHE;

        init_list_head(&vmr->cow_private_pages);

        return vmr;
}

static void free_cow_private_page(struct cow_private_page *record)
{
        kfree(record->page);
        kfree(record);
}

int vmregion_record_cow_private_page(struct vmregion *vmr, vaddr_t vaddr,
                                     void *private_page)
{
        struct cow_private_page *record;

        record = kmalloc(sizeof(*record));
        if (!record)
                return -ENOMEM;
        record->vaddr = vaddr;
        record->page = private_page;
        list_add(&record->node, &vmr->cow_private_pages);
        return 0;
}

static void free_vmregion(struct vmregion *vmr)
{
        struct cow_private_page *cur_record = NULL, *tmp = NULL;

        for_each_in_list_safe (cur_record, tmp, node, &vmr->cow_private_pages) {
                free_cow_private_page(cur_record);
        }
        list_del(&vmr->mapping_list_node);
        kfree((void *)vmr);
}

/*
 * Return value:
 * -1: node1 (vm range1) < node2 (vm range2)
 * 0: overlap
 * 1: node1 > node2
 */
static bool cmp_two_vmrs(const struct rb_node *node1,
                         const struct rb_node *node2)
{
        struct vmregion *vmr1, *vmr2;
        vaddr_t vmr1_start, vmr1_end, vmr2_start;

        vmr1 = rb_entry(node1, struct vmregion, tree_node);
        vmr2 = rb_entry(node2, struct vmregion, tree_node);

        vmr1_start = vmr1->start;
        vmr1_end = vmr1_start + vmr1->size - 1;

        vmr2_start = vmr2->start;

        /* vmr1 < vmr2 */
        if (vmr1_end < vmr2_start)
                return true;

        /* vmr1 > vmr2 or vmr1 and vmr2 overlap */
        return false;
}

struct va_range {
        vaddr_t start;
        vaddr_t end;
};

/*
 * Return value:
 * -1: va_range < node (vmr)
 *  0: overlap
 *  1: va_range > node
 */
static int cmp_vmr_and_range(const void *va_range, const struct rb_node *node)
{
        struct vmregion *vmr;
        vaddr_t vmr_start, vmr_end;

        vmr = rb_entry(node, struct vmregion, tree_node);
        vmr_start = vmr->start;
        vmr_end = vmr_start + vmr->size - 1;

        struct va_range *range = (struct va_range *)va_range;
        /* range < vmr */
        if (range->end < vmr_start)
                return -1;

        /* range > vmr */
        if (range->start > vmr_end)
                return 1;

        /* range and vmr overlap */
        return 0;
}

/*
 * Return value:
 * -1: va < node (vmr)
 *  0: va belongs to node
 *  1: va > node
 */
__maybe_unused static int cmp_vmr_and_va(const void *va,
                                         const struct rb_node *node)
{
        struct vmregion *vmr;
        vaddr_t vmr_start, vmr_end;

        vmr = rb_entry(node, struct vmregion, tree_node);
        vmr_start = vmr->start;
        vmr_end = vmr_start + vmr->size - 1;

        if ((vaddr_t)va < vmr_start)
                return -1;

        if ((vaddr_t)va > vmr_end)
                return 1;

        return 0;
}

/* Returns 0 when no intersection detected. */
static int check_vmr_intersect(struct vmspace *vmspace,
                               struct vmregion *vmr_to_add)
{
        struct va_range range;

        range.start = vmr_to_add->start;
        range.end = range.start + vmr_to_add->size - 1;

        struct rb_node *res;
        res = rb_search(
                &vmspace->vmr_tree, (const void *)&range, cmp_vmr_and_range);
        /*
         * If rb_search returns NULL,
         * the vmr_to_add will not overlap with any existing vmr.
         */
        return (res == NULL) ? 0 : 1;
}

static int add_vmr_to_vmspace(struct vmspace *vmspace, struct vmregion *vmr)
{
        if (check_vmr_intersect(vmspace, vmr) != 0) {
                kwarn("%s: vmr overlap.\n", __func__);
                return -EINVAL;
        }

        list_add(&vmr->list_node, &vmspace->vmr_list);
        rb_insert(&vmspace->vmr_tree, &vmr->tree_node, cmp_two_vmrs);
        vmr->vmspace = vmspace;
        return 0;
}

/* The @vmr is only removed but not freed. */
static void remove_vmr_from_vmspace(struct vmspace *vmspace,
                                    struct vmregion *vmr)
{
        if (check_vmr_intersect(vmspace, vmr) != 0) {
                rb_erase(&vmspace->vmr_tree, &vmr->tree_node);
                list_del(&vmr->list_node);
        }
}

static void del_vmr_from_vmspace(struct vmspace *vmspace, struct vmregion *vmr)
{
        remove_vmr_from_vmspace(vmspace, vmr);
        free_vmregion(vmr);
}

static int fill_page_table(struct vmspace *vmspace, struct vmregion *vmr)
{
        size_t pm_size;
        paddr_t pa;
        vaddr_t va;
        int ret;
        long rss;

        pm_size = vmr->pmo->size;
        pa = vmr->pmo->start;
        va = vmr->start;

        lock(&vmspace->pgtbl_lock);
        ret = map_range_in_pgtbl(
                vmspace->pgtbl, va, pa, pm_size, vmr->perm, &rss);
        vmspace->rss += rss;
        unlock(&vmspace->pgtbl_lock);

        return ret;
}

/* In the beginning, a vmspace ran on zero CPU */
static inline void reset_history_cpus(struct vmspace *vmspace)
{
        int i;

        for (i = 0; i < PLAT_CPU_NUM; ++i)
                vmspace->history_cpus[i] = 0;
}

int vmspace_init(struct vmspace *vmspace, unsigned long pcid)
{
        init_list_head(&vmspace->vmr_list);
        init_rb_root(&vmspace->vmr_tree);

        /* Allocate the root page table page */
        vmspace->pgtbl = get_pages(0);
        BUG_ON(vmspace->pgtbl == NULL);
        memset((void *)vmspace->pgtbl, 0, PAGE_SIZE);
        vmspace->pcid = pcid;

        /* Architecture-dependent initialization */
        arch_vmspace_init(vmspace);

        /*
         * Note: acquire vmspace_lock before pgtbl_lock
         * when locking them together.
         */
        lock_init(&vmspace->vmspace_lock);
        lock_init(&vmspace->pgtbl_lock);

        /* The vmspace does not run on any CPU for now */
        reset_history_cpus(vmspace);

        vmspace->heap_boundary_vmr = NULL;

        return 0;
}

void vmspace_deinit(void *ptr)
{
        struct vmspace *vmspace;
        struct vmregion *vmr = NULL;
        struct vmregion *tmp;

        vmspace = (struct vmspace *)ptr;

        /*
         * Free each vmregion in vmspace->vmr_list.
         * Only invoked when a process exits. No need to acquire the lock.
         */
        for_each_in_list_safe (vmr, tmp, list_node, &vmspace->vmr_list) {
                free_vmregion(vmr);
        }

        free_page_table(vmspace->pgtbl);

        /* TLB flush (PCID reusing) */
        flush_tlb_by_vmspace(vmspace);
}

int vmspace_map_range(struct vmspace *vmspace, vaddr_t va, size_t len,
                      vmr_prop_t flags, struct pmobject *pmo)
{
        struct vmregion *vmr;
        int ret;

        if (len == 0)
                return 0;

        /* Align a vmr to PAGE_SIZE */
        va = ROUND_DOWN(va, PAGE_SIZE);
        len = ROUND_UP(len, PAGE_SIZE);

        lock(&vmspace->vmspace_lock);

        vmr = alloc_vmregion(va, len, 0, flags, pmo);
        if (!vmr) {
                ret = -ENOMEM;
                goto out_fail;
        }

        /*
         * Each operation on the vmspace should be protected by
         * the per-vmspace lock, i.e., vmspace_lock.
         */
        ret = add_vmr_to_vmspace(vmspace, vmr);

        if (ret < 0) {
                kdebug("add_vmr_to_vmspace fails\n");
                goto out_free_vmr;
        }

        /* Eager mapping for the following pmo types and otherwise lazy mapping.
         */
        if ((pmo->type == PMO_DATA) || (pmo->type == PMO_DATA_NOCACHE)
            || (pmo->type == PMO_DEVICE))
                fill_page_table(vmspace, vmr);

        unlock(&vmspace->vmspace_lock);
        return 0;

out_free_vmr:
        free_vmregion(vmr);
out_fail:
        unlock(&vmspace->vmspace_lock);
        return ret;
}

static void __vmspace_unmap_range(struct vmspace *vmspace, vaddr_t va,
                                  size_t len)
{
        struct vmregion *vmr;
        size_t cur_size = 0;

        while (cur_size < len) {
                vmr = find_vmr_for_va(vmspace, va);
                va += vmr->size;
                cur_size += vmr->size;
                del_vmr_from_vmspace(vmspace, vmr);
        }
}

static void __vmspace_unmap_range_pgtbl(struct vmspace *vmspace, vaddr_t va,
                                        size_t len)
{
        if (len != 0) {
                long rss = 0;
                lock(&vmspace->pgtbl_lock);
                unmap_range_in_pgtbl(vmspace->pgtbl, va, len, &rss);
                vmspace->rss += rss;
                unlock(&vmspace->pgtbl_lock);
                flush_tlb_by_range(vmspace, va, len);
        }
}

static int check_unmap(struct vmspace *vmspace, vaddr_t va, size_t len,
                       struct pmobject *pmo)
{
        struct vmregion *vmr;
        size_t cur_size = 0;
        int ret = 0;

        while (cur_size < len) {
                vmr = find_vmr_for_va(vmspace, va);

                if (!vmr) {
                        ret = -EINVAL;
                        kwarn("No vmr found when unmapping.\n");
                        break;
                }
                if ((va > vmr->start) || (len - cur_size < vmr->size)) {
                        ret = -EINVAL;
                        kwarn("Only support unmapping a whole vmregion now.\n");
                        break;
                }
                if (pmo != NULL && vmr->pmo != pmo) {
                        ret = -EINVAL;
                        kwarn("Requested vmr and pmo not match.\n");
                        break;
                }

                va += vmr->size;
                cur_size += vmr->size;
        }

        return ret;
}

/*
 * Unmap routine: unmap a virtual memory range.
 * Current limitation: only supports unmap multiple whole existing vmrs.
 */
int vmspace_unmap_range(struct vmspace *vmspace, vaddr_t va, size_t len)
{
        int ret = 0;

        lock(&vmspace->vmspace_lock);

        if (check_unmap(vmspace, va, len, NULL)) {
                ret = -EINVAL;
                goto out_unlock;
        }

        __vmspace_unmap_range(vmspace, va, len);
        unlock(&vmspace->vmspace_lock);

        /* Remove the potential mappings in the page table. */
        __vmspace_unmap_range_pgtbl(vmspace, va, len);
        return 0;

out_unlock:
        unlock(&vmspace->vmspace_lock);
        return ret;
}

int vmspace_unmap_pmo(struct vmspace *vmspace, vaddr_t va, size_t len,
                      struct pmobject *pmo)
{
        int ret = 0;

        lock(&vmspace->vmspace_lock);

        if (check_unmap(vmspace, va, pmo->size, pmo)) {
                ret = -EINVAL;
                goto out_unlock;
        }

        __vmspace_unmap_range(vmspace, va, pmo->size);
        unlock(&vmspace->vmspace_lock);

        /* Remove the potential mappings in the page table. */
        __vmspace_unmap_range_pgtbl(vmspace, va, pmo->size);
        return 0;

out_unlock:
        unlock(&vmspace->vmspace_lock);
        return ret;
}

/* This function should be surrounded with the vmspace_lock. */
__maybe_unused struct vmregion *find_vmr_for_va(struct vmspace *vmspace,
                                                vaddr_t addr)
{
        /* LAB 2 TODO 6 BEGIN */
        /* Hint: Find the corresponding vmr for @addr in @vmspace */
        /* BLANK BEGIN */
        return NULL;
        /* BLANK END */
        /* LAB 2 TODO 6 END */
}

/*
 * Split a vmr at address split_vaddr to two different vmrs.
 * Special case when we split heap vmr.
 */
int split_vmr_locked(struct vmspace *vmspace, struct vmregion *old_vmr,
                     vaddr_t split_vaddr)
{
        struct vmregion *new_vmr;
        vaddr_t new_vmr_start;
        size_t old_vmr_size, new_vmr_size, new_vmr_offset;
        struct cow_private_page *cur_record = NULL, *tmp = NULL;

        if ((split_vaddr <= old_vmr->start)
            || (split_vaddr >= old_vmr->start + old_vmr->size)
            || (split_vaddr % PAGE_SIZE))
                return -EINVAL;

        old_vmr_size = split_vaddr - old_vmr->start;
        new_vmr_start = split_vaddr;
        new_vmr_size = old_vmr->size - old_vmr_size;
        new_vmr_offset = old_vmr->offset + old_vmr_size;

        new_vmr = alloc_vmregion(new_vmr_start,
                                 new_vmr_size,
                                 new_vmr_offset,
                                 old_vmr->perm,
                                 old_vmr->pmo);
        if (new_vmr == NULL) {
                return -ENOMEM;
        }

        for_each_in_list_safe (
                cur_record, tmp, node, &old_vmr->cow_private_pages) {
                if (cur_record->vaddr >= split_vaddr) {
                        list_del(&cur_record->node);
                        list_add(&cur_record->node,
                                 &new_vmr->cow_private_pages);
                }
        }

        /*
         * If we are spliting heap_boundary_vmr, we need to update it to the new
         * boundary vmr. Otherwise, we cannot get right boundary of the
         * heap in sys_handle_brk.
         */
        if (old_vmr == vmspace->heap_boundary_vmr) {
                vmspace->heap_boundary_vmr = new_vmr;
        }

        remove_vmr_from_vmspace(vmspace, old_vmr);
        old_vmr->size = old_vmr_size;
        add_vmr_to_vmspace(vmspace, old_vmr);
        add_vmr_to_vmspace(vmspace, new_vmr);
        return 0;
}

/* Each process has one heap_vmr. */
struct vmregion *init_heap_vmr(struct vmspace *vmspace, vaddr_t va,
                               struct pmobject *pmo)
{
        return alloc_vmregion(va, 0, 0, VMR_READ | VMR_WRITE, pmo);
}

int adjust_heap_vmr(struct vmspace *vmspace, unsigned long add_len)
{
        struct vmregion *vmr;
        struct vmregion tmp_vmr;

        vmr = vmspace->heap_boundary_vmr;

        /* check if the heap can be expanded */
        tmp_vmr.start = vmr->start + vmr->size;
        tmp_vmr.size = add_len;
        if (check_vmr_intersect(vmspace, &tmp_vmr)) {
                /* conflict, do nothing */
                return -EINVAL;
        }

        remove_vmr_from_vmspace(vmspace, vmr);
        vmr->size += add_len;
        vmr->pmo->size += add_len;
        return add_vmr_to_vmspace(vmspace, vmr);
}

/* Dumping all the vmrs of one vmspace. */
void kprint_vmr(struct vmspace *vmspace)
{
        struct rb_node *node;
        struct vmregion *vmr;
        vaddr_t start, end;

        /* rb_for_each will iterate the vmrs in order. */
        rb_for_each(&vmspace->vmr_tree, node)
        {
                vmr = rb_entry(node, struct vmregion, tree_node);
                start = vmr->start;
                end = start + vmr->size;
                kinfo("[%p] [vmregion] start=%p end=%p. vmr->pmo->type=%d\n",
                      vmspace,
                      start,
                      end,
                      vmr->pmo->type);
        }
}

/*
 * Note that lock/atomic_ops are not required here
 * because only CPU X will modify (record/clear) history_cpus[X].
 */
void record_history_cpu(struct vmspace *vmspace, unsigned int cpuid)
{
        BUG_ON(cpuid >= PLAT_CPU_NUM);
        vmspace->history_cpus[cpuid] = 1;
}

void clear_history_cpu(struct vmspace *vmspace, unsigned int cpuid)
{
        BUG_ON(cpuid >= PLAT_CPU_NUM);
        vmspace->history_cpus[cpuid] = 0;
}
