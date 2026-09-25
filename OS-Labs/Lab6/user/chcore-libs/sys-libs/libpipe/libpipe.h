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

#pragma once

#include <chcore/type.h>

struct pipe;

struct pipe *create_pipe_from_pmo(u32 size, cap_t pmo_cap);
struct pipe *create_pipe_from_vaddr(u32 size, void *data);
void del_pipe(struct pipe *pp);
void pipe_init(const struct pipe *pp);
u32 pipe_read(const struct pipe *pp, void *buf, u32 n);
u32 pipe_read_exact(const struct pipe *pp, void *buf, u32 n);
u32 pipe_write(const struct pipe *pp, const void *buf, u32 n);
u32 pipe_write_exact(const struct pipe *pp, const void *buf, u32 n);
bool pipe_is_empty(const struct pipe *pp);
bool pipe_is_full(const struct pipe *pp);
