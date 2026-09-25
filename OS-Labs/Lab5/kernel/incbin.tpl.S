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

/* Kernel binary include template. */

        .section .rodata
        .align 4
        .globl binary_${binary_name}_start
binary_${binary_name}_start:
        .incbin "${binary_path}"
binary_${binary_name}_end:
        .globl binary_${binary_name}_size
binary_${binary_name}_size:
#if __SIZEOF_POINTER__ == 4
        .word binary_${binary_name}_end - binary_${binary_name}_start
#else
        .quad binary_${binary_name}_end - binary_${binary_name}_start
#endif
