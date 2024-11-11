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

#include <common/asm.h>
#include <common/vars.h>

    .macro  read_ctr, reg
        mrs     \reg, ctr_el0
        nop
    .endm

    .macro  dcache_line_size, reg, tmp
	read_ctr        \tmp
        /* cache line size encoding */
	ubfm            \tmp, \tmp, #16, #19
        /* bytes per word */
	mov             \reg, #4
        /* actual cache line size */
	lsl             \reg, \reg, \tmp
    .endm

    .macro dcache_by_line_op op, domain, kaddr, size, tmp1, tmp2
        dcache_line_size \tmp1, \tmp2
        add     \size, \kaddr, \size
        sub     \tmp2, \tmp1, #1
        bic     \kaddr, \kaddr, \tmp2
1:
        dc      \op, \kaddr
        add     \kaddr, \kaddr, \tmp1
        cmp     \kaddr, \size
        b.lo    1b
        dsb     \domain
    .endm

BEGIN_FUNC(flush_dcache_area)
	dcache_by_line_op civac, sy, x0, x1, x2, x3
	ret
END_FUNC(flush_dcache_area)

BEGIN_FUNC(put32)
    str w1,[x0]
    ret
END_FUNC(put32)

BEGIN_FUNC(get32)
    ldr w0,[x0]
    ret
END_FUNC(get32)

BEGIN_FUNC(put64)
    str x1,[x0]
    ret
END_FUNC(put64)

BEGIN_FUNC(get64)
    ldr x0,[x0]
    ret
END_FUNC(get64)
