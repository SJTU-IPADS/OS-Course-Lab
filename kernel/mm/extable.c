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

#include <mm/extable.h>
#include <common/types.h>
#include <common/kprint.h>

const struct exception_table_entry *search_extable(vaddr_t addr)
{
    const struct exception_table_entry *e;

    kdebug("searching the extable, start at 0x%lx, end at 0x%lx\n", _extable_start, _extable_end);
    for (e = _extable_start; e != _extable_end && e->insn != 0; e++) {
        // kinfo("insn: %lx, fixup: %lx\n", e->insn, e->fixup);
        if (e->insn == addr) {
            return e;
        }
    }
    
    return NULL;
}

bool fixup_exception(vaddr_t addr, vaddr_t *fix_addr)
{
    const struct exception_table_entry *e;

    e = search_extable(addr);
    if (!e) {
        kinfo("fail to fix up exception at address: %lx\n", addr);
        return false;
    }

    kdebug("find fixup! fault instr addr: %lx, fix instr addr: %lx\n", addr, e->fixup);
    *fix_addr = e->fixup;

    return true;
}
