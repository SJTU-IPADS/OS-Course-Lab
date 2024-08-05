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
#ifndef UAPI_MEMORY_H
#define UAPI_MEMORY_H

/* pmo types */
#ifndef __ASSEMBLER__
typedef unsigned pmo_type_t;
#endif
#define PMO_ANONYM       0 /* lazy allocation */
#define PMO_DATA         1 /* immediate allocation */
#define PMO_FILE         2 /* file backed */
#define PMO_SHM          3 /* shared memory */
#define PMO_USER_PAGER   4 /* support user pager */
#define PMO_DEVICE       5 /* memory mapped device registers */
#define PMO_DATA_NOCACHE 6 /* non-cacheable immediate allocation */

#define PMO_FORBID     10 /* Forbidden area: avoid overflow */

/* vmr permissions */
#ifndef __ASSEMBLER__
typedef unsigned long vmr_prop_t;
#endif
#define VMR_READ    (1 << 0)
#define VMR_WRITE   (1 << 1)
#define VMR_EXEC    (1 << 2)
#define VMR_DEVICE  (1 << 3)
#define VMR_NOCACHE (1 << 4)
#define VMR_COW     (1 << 5)

#endif /* UAPI_MEMORY_H */
