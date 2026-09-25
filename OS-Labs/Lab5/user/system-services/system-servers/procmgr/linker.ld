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

ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);   /* FLAGS: R-X */
    data PT_LOAD FLAGS(6);   /* FLAGS: RW- */
    rodata PT_LOAD FLAGS(4); /* FLAGS: R-- */
}

SECTIONS
{
    . = 0x400000;

    .text : ALIGN(4K) {
        *(.text*)
    } : text

    .data : ALIGN(4K) {
        *(.data*)
    } : data

    .bss : ALIGN(4K) {
        *(.bss*)
    } : data

    .init_array : ALIGN(4K) {
        __init_array_start = .;
        *(SORT_BY_INIT_PRIORITY(.init_array.*)) *(.init_array) 
        *(SORT_BY_INIT_PRIORITY(.ctors.*))
    } : data
    __init_array_end = .;

    .fini_array : ALIGN(4K) {
        __fini_array_start = .;
        *(SORT_BY_INIT_PRIORITY(.fini_array.*)) *(.fini_array) 
        *(SORT_BY_INIT_PRIORITY(.dtors.*))
    } : data
    __fini_array_end = .;

    .rodata : ALIGN(4K) {
        *(.rodata*)
    } : rodata
}
