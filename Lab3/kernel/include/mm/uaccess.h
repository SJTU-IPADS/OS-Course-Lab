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

#ifndef MM_UACCESS_H
#define MM_UACCESS_H

#include <object/cap_group.h>

int copy_from_user(void *kbuf, void *ubuf, size_t size);
int copy_to_user(void *ubuf, void *kbuf, size_t size);
int copy_from_user_remote(struct cap_group *cap_group, void *kernel_buf,
			  void *user_buf, size_t size);
int copy_to_user_remote(struct cap_group *cap_group, void *user_buf,
			void *kernel_buf, size_t size);

/*
 * For x86_64:
 * In 64-bit mode, an address is considered to be in canonical form
 * if address bits 63 through to the most-significant implemented bit
 * by the microarchitecture are set to either all ones or all zeros.
 * The kernel and user share the 48-bit address (0~2^48-1).
 * As usual, we let the kernel use the top half and the user use the
 * bottom half.
 * So, the user address is 0 ~ 2^47-1 (USER_SPACE_END).
 *
 *
 * For aarch64:
 * With 4-level page tables:
 * TTBR0_EL1 translates 0 ~ 2^48-1 .
 * TTBR1_EL1 translates (0xFFFF) + 0 ~ 2^48-1.
 * But, we only use 0 ~ USER_SPACE_END (as x86_64).
 * With 3-level page tables:
 * The same as x86_64.

#if __SIZEOF_POINTER__ == 4
#define USER_SPACE_END  (0x80000000UL)
#else
#define USER_SPACE_END  (0x800000000000UL)
#endif

*/

#include <common/vars.h>

static inline int check_user_addr_range(vaddr_t start, size_t len)
{
        if ((start + len) >= KBASE)
                return -1;
        return 0;
}

#endif /* MM_UACCESS_H */