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

#define _GNU_SOURCE
#include <string.h>
#include "pthread_impl.h"
#include "syscall.h"

hidden long __cancel();

long __syscall_cp_c(syscall_arg_t nr,
                    syscall_arg_t u, syscall_arg_t v, syscall_arg_t w,
                    syscall_arg_t x, syscall_arg_t y, syscall_arg_t z)
{
	pthread_t self;
	long r;
	int st;

	if ((st=(self=__pthread_self())->canceldisable)
	    && (st==PTHREAD_CANCEL_DISABLE || nr==SYS_close))
		return __syscall(nr, u, v, w, x, y, z);

	/* 
	 * FIXME: All ChCore syscall should use __syscall rather than __syscall_cp_asm 
	 * May cause some problem here... 
	 */
	// r = __syscall_cp_asm(&self->cancel, nr, u, v, w, x, y, z);
	r = __syscall(nr, u, v, w, x, y, z);

	if (r==-EINTR && nr!=SYS_close && self->cancel &&
	    self->canceldisable != PTHREAD_CANCEL_DISABLE)
		r = __cancel();
	return r;
}