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

#ifndef __RK_ATAGS_H_
#define __RK_ATAGS_H_

#include <common/types.h>
#include <common/macro.h>
#include <arch/mmu.h>

#define __packed __attribute__((packed))

#define MAGIC_CORE    0x54410001
#define MAGIC_NONE    0x00000000
#define MAGIC_MIN     0x5441004f
#define MAGIC_TOS_MEM 0x54410053
#define MAGIC_MAX     0x544100ff

#define ATAGS_SIZE   (0x2000) /* 8K */
#define ATAGS_OFFSET (0x200000 - ATAGS_SIZE) /* [2M-8K, 2M] */

#define CONFIG_SYS_SDRAM_BASE 0
#define ATAGS_PHYS_BASE       (CONFIG_SYS_SDRAM_BASE + ATAGS_OFFSET)
#define ATAGS_VIRT_BASE       (phys_to_virt(ATAGS_PHYS_BASE))
#define ATAGS_VIRT_END        (ATAGS_VIRT_BASE + ATAGS_SIZE)

struct tag_tos_mem {
        u32 version;
        struct {
                char name[8];
                u64 phy_addr;
                u32 size;
                u32 flags;
        } tee_mem;

        struct {
                char name[8];
                u64 phy_addr;
                u32 size;
                u32 flags;
        } drm_mem;

        u64 reserved[7];
        u32 reserved1;
        u32 hash;
} __packed;

struct tag_core {
        u32 flags;
        u32 pagesize;
        u32 rootdev;
} __packed;

struct tag_header {
        u32 size; /* bytes = size * 4 */
        u32 magic;
} __packed;

struct tag {
        struct tag_header hdr;
        union {
                struct tag_core core;
                struct tag_tos_mem tos_mem;
        } u;
} ALIGN(4);

#define tag_next(t)           ((struct tag *)((u32 *)(t) + (t)->hdr.size))
#define tag_size(type)        ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)
#define for_each_tag(t, base) for (t = base; t->hdr.size; t = tag_next(t))

int set_tos_mem_tag(void *tagdata);

#endif
