/* unit test */
#include "minunit.h"

#include <stdlib.h>

#include <common/list.h>
#include <common/rbtree.h>
#include <../mm/vmspace.c>

void *kmalloc(unsigned long size)
{
        return malloc(size);
}

void kfree(void *ptr)
{
        free(ptr);
}

void *get_pages(int order)
{
        return malloc(PAGE_SIZE);
}

int unmap_range_in_pgtbl(void *pgtbl, vaddr_t va, size_t len, long *rss)
{
        return 0;
}
void arch_vmspace_init(struct vmspace *vmspace)
{
        return;
}
void free_page_table(void *pgtbl)
{
        return;
}
void flush_tlb_by_range(struct vmspace *vmspace, vaddr_t start_va, size_t len)
{
        return;
}
void flush_tlb_by_vmspace(struct vmspace *vmspace)
{
        return;
}
int map_range_in_pgtbl(void *pgtbl, vaddr_t va, paddr_t pa, size_t len,
                       vmr_prop_t flags, long *rss)
{
        return 0;
}

void check_vmr_prop(struct vmregion *vmr, vaddr_t start, size_t size,
                    size_t offset, vmr_prop_t perm, struct pmobject *pmo)
{
        mu_check(vmr->start == start);
        mu_check(vmr->size == size);
        mu_check(vmr->offset == offset);
        mu_check(vmr->perm == perm);
        mu_check(vmr->pmo == pmo);
}

void check_vmr_cow(struct vmregion *vmr, vaddr_t *addr, int num)
{
        struct cow_private_page *cur_cow = NULL, *tmp_cow = NULL;
        int cow_cnt = 0;

        for_each_in_list_safe (
                cur_cow, tmp_cow, node, &vmr->cow_private_pages) {
                mu_check(cow_cnt < num);
                mu_check(cur_cow->page == (void *)addr[cow_cnt]);
                mu_check(cur_cow->vaddr == addr[cow_cnt]);
                cow_cnt++;
        }
        mu_check(cow_cnt == num);
}

MU_TEST(test_vmr_split)
{
        struct vmspace *vmspace;
        struct vmregion *vmr0, *vmr1, *vmr2, *vmr3;
        struct vmregion *heap_vmr0, *heap_vmr1;
        struct vmregion *cur_vmr = NULL, *tmp_vmr = NULL;
        struct pmobject *pmo;
        int vmr_cnt = 0;
        vaddr_t addr[10];

        /* initialize vmspace and vmrs */
        vmspace = kmalloc(sizeof(struct vmspace));
        vmspace_init(vmspace, 0);

        pmo = kmalloc(sizeof(struct pmobject));
        pmo->type = PMO_ANONYM;
        init_list_head(&pmo->mapping_list);

        vmr0 = alloc_vmregion(
                0x50000000, 10 * (size_t)PAGE_SIZE, 0, VMR_DEVICE, pmo);
        vmregion_record_cow_private_page(vmr0, 0x50001000, (void *)0x50001000);
        vmregion_record_cow_private_page(vmr0, 0x50006000, (void *)0x50006000);
        vmregion_record_cow_private_page(vmr0, 0x50009000, (void *)0x50009000);

        heap_vmr0 = alloc_vmregion(0x70000000,
                                   10 * (size_t)PAGE_SIZE,
                                   0,
                                   (VMR_READ | VMR_WRITE),
                                   pmo);
        vmspace->heap_boundary_vmr = heap_vmr0;

        add_vmr_to_vmspace(vmspace, vmr0);
        add_vmr_to_vmspace(vmspace, heap_vmr0);

        /* split vmrs */
        split_vmr_locked(vmspace, vmr0, 0x50003000);
        vmr1 = find_vmr_for_va(vmspace, 0x50003000);
        split_vmr_locked(vmspace, vmr1, 0x50005000);
        vmr2 = find_vmr_for_va(vmspace, 0x50005000);
        split_vmr_locked(vmspace, vmr2, 0x50008000);
        vmr3 = find_vmr_for_va(vmspace, 0x50008000);
        split_vmr_locked(vmspace, heap_vmr0, 0x70004000);
        heap_vmr1 = find_vmr_for_va(vmspace, 0x70004000);

        /* check data */
        for_each_in_list_safe (
                cur_vmr, tmp_vmr, list_node, &vmspace->vmr_list) {
                vmr_cnt++;
                if (cur_vmr == vmr0) {
                        check_vmr_prop(cur_vmr,
                                       0x50000000,
                                       3 * (size_t)PAGE_SIZE,
                                       0,
                                       VMR_DEVICE,
                                       pmo);
                        addr[0] = 0x50001000;
                        check_vmr_cow(cur_vmr, addr, 1);
                        continue;
                }
                if (cur_vmr == vmr1) {
                        check_vmr_prop(cur_vmr,
                                       0x50003000,
                                       2 * (size_t)PAGE_SIZE,
                                       0x3000,
                                       VMR_DEVICE,
                                       pmo);
                        check_vmr_cow(cur_vmr, addr, 0);
                        continue;
                }
                if (cur_vmr == vmr2) {
                        check_vmr_prop(cur_vmr,
                                       0x50005000,
                                       3 * (size_t)PAGE_SIZE,
                                       0x5000,
                                       VMR_DEVICE,
                                       pmo);
                        addr[0] = 0x50006000;
                        check_vmr_cow(cur_vmr, addr, 1);
                        continue;
                }
                if (cur_vmr == vmr3) {
                        check_vmr_prop(cur_vmr,
                                       0x50008000,
                                       2 * (size_t)PAGE_SIZE,
                                       0x8000,
                                       VMR_DEVICE,
                                       pmo);
                        addr[0] = 0x50009000;
                        check_vmr_cow(cur_vmr, addr, 1);
                        continue;
                }
                if (cur_vmr == heap_vmr0) {
                        check_vmr_prop(cur_vmr,
                                       0x70000000,
                                       4 * (size_t)PAGE_SIZE,
                                       0,
                                       (VMR_READ | VMR_WRITE),
                                       pmo);
                        check_vmr_cow(cur_vmr, addr, 0);
                        continue;
                }
                if (cur_vmr == heap_vmr1) {
                        check_vmr_prop(cur_vmr,
                                       0x70004000,
                                       6 * (size_t)PAGE_SIZE,
                                       0x4000,
                                       (VMR_READ | VMR_WRITE),
                                       pmo);
                        check_vmr_cow(cur_vmr, addr, 0);
                        continue;
                }
                mu_fail("unexpected vmr in vmspace");
        }
        mu_check(vmr_cnt == 6);
        mu_check(vmspace->heap_boundary_vmr == heap_vmr1);
}

MU_TEST_SUITE(test_suite)
{
        MU_RUN_TEST(test_vmr_split);
}

int main(int argc, char *argv[])
{
        MU_RUN_SUITE(test_suite);
        MU_REPORT();
        return minunit_status;
}
