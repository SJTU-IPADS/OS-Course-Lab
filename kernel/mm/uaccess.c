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

#include <arch/mmu.h>
#include <common/util.h>
#include <common/vars.h>
#include <common/types.h>
#include <common/macro.h>
#include <mm/mm.h>

static int transform_vaddr(struct cap_group *cap_group, char *user_buf, vaddr_t *kva)
{
	struct vmspace *vmspace;
	struct vmregion *vmr;
	struct pmobject *pmo;
	int ret = 0;
	long rss;
	unsigned long offset, index;
	vaddr_t va; /* Aligned va of user_buf */
	paddr_t pa;

	vmspace = obj_get(cap_group, VMSPACE_OBJ_ID, TYPE_VMSPACE);
	BUG_ON(vmspace == NULL);

	/* Prevent concurrent operations on the vmspace */
	lock(&vmspace->vmspace_lock);

	vmr = find_vmr_for_va(vmspace, (vaddr_t)user_buf);
	/* Target user address is not valid (not mapped before) */
	if (!vmr || !vmr->pmo) {
		ret = -EINVAL;
		goto out;
	}
	pmo = vmr->pmo;

	/*
	 * A special case: the address is mapped with an anonymous pmo
	 * which is not directly mapped by the user.
	 * And, kernel touches it first.
	 *
	 * To prevent pgfault, kernel maps the page first if necessary.
	 *
	 * pmo->type is data: must be mapped
	 */
	switch (pmo->type) {
	case PMO_DATA:
	case PMO_DATA_NOCACHE: {
		/*
		 * Calculate the kva for the user_buf:
		 * kva = start_addr + offset
		 */
		*kva = phys_to_virt(vmr->pmo->start) +
			((vaddr_t)user_buf - vmr->start);
		break;
	}
	case PMO_ANONYM: {
		va = ROUND_DOWN((unsigned long)user_buf, PAGE_SIZE);
		offset = va - vmr->start;
		/* Boundary check */
		BUG_ON(offset >= pmo->size);
		index = offset / PAGE_SIZE;
		/* Get the physical page for va according to the radix tree in the pmo */
		pa = get_page_from_pmo(pmo, index);

		if (pa == 0) {
			/* No physical page allocated to it before */
			void *new_va = get_pages(0);
			BUG_ON(new_va == NULL);
			pa = virt_to_phys(new_va);
			BUG_ON(pa == 0);
			memset((void *)phys_to_virt(pa), 0, PAGE_SIZE);

			commit_page_to_pmo(pmo, index, pa);

			lock(&vmspace->pgtbl_lock);
			map_range_in_pgtbl(vmspace->pgtbl, va, pa, PAGE_SIZE,
					   vmr->perm, &rss);
			vmspace->rss += rss;
			unlock(&vmspace->pgtbl_lock);
		}
		/*
		 * Return: start address of the newly allocated page +
		 *         offset in the page (last 12 bits).
		 */
		*kva = phys_to_virt(pa) +
			(((vaddr_t)user_buf) & PAGE_MASK);
		break;
	}
	case PMO_SHM:
	case PMO_FILE: {
		va = ROUND_DOWN((unsigned long)user_buf, PAGE_SIZE);
		offset = va - vmr->start;

		/*
		 * Boundary check.
		 *
		 * No boundary check for PMO_FILE because a file can
		 * be mapped to a larger (larger than the file size) vmr.
		 * (allowed in Linux)
		 *
		 * TODO: make the behaviour more accurate in ChCore
		 */
		if (pmo->type == PMO_SHM && offset >= pmo->size) {
			kinfo("boundary check failed\n");
			return -EINVAL;
		}

		index = offset / PAGE_SIZE;
		/* Get the physical page for va according to the radix tree in the pmo */
		pa = get_page_from_pmo(pmo, index);

		if (pa == 0) {
                        /*
                         * For PMO_FILE, data may reside in memory but are not
                         * indexed in pmo.
                         */
			query_in_pgtbl(vmspace->pgtbl, va, &pa, NULL);
			if (pa == 0) {
				/*
				 * No physical page allocated to it before.
				 * SHM should not be accessed by kernel first.
				 */
				return -EINVAL;
			}
		}
		/*
		 * Return: start address of the newly allocated page +
		 *         offset in the page (last 12 bits).
		 */
		*kva = phys_to_virt(pa) +
			(((vaddr_t)user_buf) & PAGE_MASK);
		break;
	}
	default: {
		kinfo("bug: kernel accessing pmo type: %d\n", pmo->type);
		ret = -EINVAL;
		break;
	}
	}

out:
	unlock(&vmspace->vmspace_lock);

	obj_put(vmspace);
	return ret;
}

int copy_from_user_remote(struct cap_group *cap_group,
			  void *kernel_buf, void *user_buf, size_t size)
{
	vaddr_t kva = 0;
	unsigned long offset, len;

	/* Validate user_buf */
	BUG_ON((unsigned long)user_buf + size >= KBASE);

	/* For the frist user memory page */
	if (transform_vaddr(cap_group, user_buf, &kva))
		return -EINVAL;
	offset = ((vaddr_t)user_buf) & PAGE_MASK;

	/* The first memory page (maybe incomplete) */
	if (size + offset <= PAGE_SIZE) {
		memcpy(kernel_buf, (void *)kva, size);
		return 0;
	} else {
		len = PAGE_SIZE - offset;
		memcpy(kernel_buf, (void *)kva, len);
		user_buf += len;
		kernel_buf += len;
		size -= len;
	}

	/* Intermediate memory pages */
	BUG_ON(((unsigned long)user_buf % PAGE_SIZE) != 0);
	while (size > PAGE_SIZE) {
		if (transform_vaddr(cap_group, user_buf, &kva))
			return -EINVAL;
		memcpy(kernel_buf, (void *)kva, PAGE_SIZE);
		user_buf += PAGE_SIZE;
		kernel_buf += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	/* The last memory page */
	if (transform_vaddr(cap_group, user_buf, &kva))
		return -EINVAL;
	memcpy(kernel_buf, (void *)kva, size);

	return 0;
}

int copy_to_user_remote(struct cap_group *cap_group, void *user_buf,
			void *kernel_buf, size_t size)
{
	vaddr_t kva = 0;
	unsigned long offset, len;

	/* Validate user_buf */
	if((unsigned long)user_buf + size >= KBASE)
		return -EINVAL;

	/* For the frist user memory page */
	if (transform_vaddr(cap_group, user_buf, &kva))
		return -EINVAL;
	offset = ((vaddr_t)user_buf) & PAGE_MASK;

	/* The first memory page (maybe incomplete) */
	if (size + offset <= PAGE_SIZE) {
		memcpy((void *)kva, kernel_buf, size);
		return 0;
	} else {
		len = PAGE_SIZE - offset;
		memcpy((void *)kva, kernel_buf, len);
		user_buf += len;
		kernel_buf += len;
		size -= len;
	}

	/* Intermediate memory pages */
	BUG_ON(((unsigned long)user_buf % PAGE_SIZE) != 0);
	while (size > PAGE_SIZE) {
		if (transform_vaddr(cap_group, user_buf, &kva))
			return -EINVAL;
		memcpy((void *)kva, kernel_buf, PAGE_SIZE);
		user_buf += PAGE_SIZE;
		kernel_buf += PAGE_SIZE;
		size -= PAGE_SIZE;
	}

	/* The last memory page */
	if (transform_vaddr(cap_group, user_buf, &kva))
		return -EINVAL;
	memcpy((void *)kva, kernel_buf, size);
	return 0;
}
