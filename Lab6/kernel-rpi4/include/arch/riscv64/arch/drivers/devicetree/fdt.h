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

/*
 *  Copyright (c) 2013, The Regents of the University of California (Regents).
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the Regents nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
 *  SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
 *  OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
 *  BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
 *  HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
 *  MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/* Reference: riscv-pk/machine/fdt.h */

#include <common/types.h>

#define FDT_MAGIC   0xd00dfeed
#define FDT_VERSION 17

struct fdt_header {
	u32 magic;
	u32 totalsize;
	u32 off_dt_struct;
	u32 off_dt_strings;
	u32 off_mem_rsvmap;
	u32 version;
	u32 last_comp_version; /* <= 17 */
	u32 boot_cpuid_phys;
	u32 size_dt_strings;
	u32 size_dt_struct;
};

#define FDT_BEGIN_NODE 1
#define FDT_END_NODE   2
#define FDT_PROP       3
#define FDT_NOP        4
#define FDT_END        9

struct fdt_scan_node {
	const struct fdt_scan_node *parent;
	const char *name;
	int address_cells;
	int size_cells;
};

struct fdt_scan_prop {
	const struct fdt_scan_node *node;
	const char *name;
	u32 *value;
	int len; // in bytes of value
};

struct fdt_cb {
	void (*open)(const struct fdt_scan_node *node, void *extra);
	void (*prop)(const struct fdt_scan_prop *prop, void *extra);
	void (*done)(const struct fdt_scan_node *node,
		     void *extra); // last property was seen
	int (*close)(const struct fdt_scan_node *node,
		     void *extra); // -1 => delete the node + children
	void *extra;
};

/* support 8 memory segments at most */
struct mem_info {
	u64 base[8];
	u64 size[8];
	int len;
};

void parse_fdt(u32 fdt_addr);
struct mem_info *fdt_get_mem_info(void);
u64 fdt_get_timebase_frequency(void);
u64 fdt_get_plic_base_addr(void);
