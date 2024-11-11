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

// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2014, Google Inc. All rights reserved
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

/*
   MIT License

   Copyright (c) 2018 Sergey Matyukevich

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#include <common/asm.h>                             

BEGIN_FUNC(flush_idcache)
	mrs     x0, clidr_el1
	and     w3, w0, #0x07000000     // get 2x level of coherence
	lsr     w3, w3, #23
	cbz     w3, .Lfinished_inv_cache
	mov     w10, #0                 // w10 = 2x cache level
	mov     w8, #1                  // w8 = constant 1
.Lloop1_inv_cache:
	add     w2, w10, w10, lsr #1    // calculate 3x cache level
	lsr     w1, w0, w2              // extract 3 bit cache type for this level
	and     w1, w1, #0x7
	cmp     w1, #2
	b.lt    .Lskip_inv_cache            // no data or unified cache at this level
	msr     csselr_el1, x10         // select this cache level
	isb                             // synchronize change to csselr
	mrs     x1, ccsidr_el1          // w1 = ccsidr
	and     w2, w1, #7              // w2 = log2(line len) - 4
	add     w2, w2, #4              // w2 = log2(line len)
	ubfx    w4, w1, #3, #10         // w4 = max way number, right aligned
	clz     w5, w4                  // w5 = 32 - log2(ways), bit position of way in DC operand
	lsl     w9, w4, w5              // w9 = max way number, aligned to position in DC operand
	lsl     w12, w8, w5             // w12 = amount to decrement way number per iteration

.Lloop2_inv_cache:
	ubfx    w7, w1, #13, #15        // w7 = max set number, right aligned
	lsl     w7, w7, w2              // w7 = max set number, aligned to position in DC operand
	lsl     w13, w8, w2             // w13 = amount to decrement set number per iteration
.Lloop3_inv_cache:
	orr     w11, w10, w9            // w11 = combine way number and cache number
	orr     w11, w11, w7            //       and set number for DC operand
	dc      isw, x11                // data cache op
	subs    w7, w7, w13             // decrement set number
	b.ge    .Lloop3_inv_cache

	subs    x9, x9, x12             // decrement way number
	b.ge    .Lloop2_inv_cache
.Lskip_inv_cache:
	add     w10, w10, #2            // increment 2x cache level
	cmp     w3, w10
	dsb     sy                      // ensure completetion of previous cache maintainance instructions
	b.gt    .Lloop1_inv_cache
.Lfinished_inv_cache:

	// dump the instruction cache as well
	ic      iallu
	isb
	ret
END_FUNC(flush_idcache)

BEGIN_FUNC(set_ttbr0_el1)
	msr ttbr0_el1, x0
	/* flushing tlb is not needed since we've enabled asid mechanism */
	//dsb sy
	//tlbi vmalle1is
	//dsb sy
	isb
	ret
END_FUNC(set_ttbr0_el1)

