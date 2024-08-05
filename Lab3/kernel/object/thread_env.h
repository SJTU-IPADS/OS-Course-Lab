/*
 * Copyright (c) 2023 Institute of Parallel And Distributed Systems (IPADS),
 * Shanghai Jiao Tong University (SJTU) Licensed under the Mulan PSL v2. You can
 * use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *     http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE. See the
 * Mulan PSL v2 for more details.
 */

#pragma once

struct process_metadata {
        unsigned long phdr_addr;
        unsigned long phentsize;
        unsigned long phnum;
        unsigned long flags;
        unsigned long entry;
};

#define ENV_SIZE_ON_STACK   0x1000
#define ROOT_BIN_HDR_SIZE   216
#define ROOT_MEM_SIZE_OFF   0
#define ROOT_ENTRY_OFF      8
#define ROOT_FLAGS_OFF      16
#define ROOT_PHENT_SIZE_OFF 24
#define ROOT_PHNUM_OFF      32
#define ROOT_PHDR_ADDR_OFF  40
#define ROOT_PHDR_OFF       48
#define ROOT_PHENT_SIZE     56

/* Program header content(64bit) */
#define PHDR_TYPE_OFF   0
#define PHDR_FLAGS_OFF  4
#define PHDR_OFFSET_OFF 8
#define PHDR_VADDR_OFF  16
#define PHDR_PADDR_OFF  24
#define PHDR_FILESZ_OFF 32
#define PHDR_MEMSZ_OFF   40
#define PHDR_ALIGN_OFF  48

#define PHDR_FLAGS_R (1 << 2)
#define PHDR_FLAGS_W (1 << 1)
#define PHDR_FLAGS_X (1 << 0)
