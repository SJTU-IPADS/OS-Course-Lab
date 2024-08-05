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

#ifndef LIB_MEM_USAGE_INFO_TOOL_H
#define LIB_MEM_USAGE_INFO_TOOL_H

#include <common/types.h>
#include <common/list.h>
#include <common/util.h>

#define RECORD_SIZE 20

extern bool collecting_switch;

void record_mem_usage(size_t size, void *addr);
void revoke_mem_usage(void *addr);

#endif /* LIB_MEM_USAGE_INFO_TOOL_H */