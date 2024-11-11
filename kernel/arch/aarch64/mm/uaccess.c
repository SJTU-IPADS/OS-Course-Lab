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

#include <common/util.h>
#include <common/vars.h>
#include <common/types.h>
#include <common/kprint.h>
#include <common/macro.h>

/*
 * Currently, we enable EL1 (kernel) to directly access EL0 (user)  memory.
 * But, El1 cannot execute EL0 code.
 */

#ifndef FBINFER

int copy_from_user(void *kernel_buf, void *user_buf, size_t size)
{
	/* validate user_buf */
	BUG_ON((u64)user_buf >= KBASE);
	return __copy_from_user(kernel_buf, user_buf, size);
}

int copy_to_user(void *user_buf, void *kernel_buf, size_t size)
{
	/* validate user_buf */
	BUG_ON((u64)user_buf >= KBASE);
	return __copy_to_user(user_buf, kernel_buf, size);
}

#endif
