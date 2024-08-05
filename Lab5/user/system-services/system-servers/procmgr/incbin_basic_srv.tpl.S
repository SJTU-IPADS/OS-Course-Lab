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

/* Binary include template. */

        .section .rodata
        .align 4
        .globl __binary_${_binary_name}_start
__binary_${_binary_name}_start:
        .incbin "${_binary_path}"
__binary_${_binary_name}_end:
        .globl __binary_${_binary_name}_size
__binary_${_binary_name}_size:
        .quad __binary_${_binary_name}_end - __binary_${_binary_name}_start
