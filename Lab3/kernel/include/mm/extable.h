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

#ifndef MM_EXTABLE_H
#define MM_EXTABLE_H

#include <common/types.h>

struct exception_table_entry
{
    vaddr_t insn, fixup;
};

extern struct exception_table_entry _extable_start[];
extern struct exception_table_entry _extable_end[];

const struct exception_table_entry *search_extable(vaddr_t addr);

bool fixup_exception(vaddr_t address, vaddr_t *fix_addr);

#endif /* MM_EXTABLE_H */