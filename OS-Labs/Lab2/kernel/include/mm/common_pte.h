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

#ifndef MM_COMMON_PTE_H
#define MM_COMMON_PTE_H

#include <common/types.h>
#include <arch/mmu.h>

#define L0 (0)
#define L1 (1)
#define L2 (2)
#define L3 (3)

/**
 * @brief Architecture-independent PTE structure, containing some useful
 * information which is shared by all architectures.
 *
 * This struct can be used to write architecture-independent code to parse
 * or manipulate PTEs.
 */
struct common_pte_t {
        /** Physical Page Number */
        unsigned long ppn;
        /** ChCore VMR permission, architecture-independent */
        vmr_prop_t perm;
        unsigned char
                /** This PTE is valid or not */
                valid : 1,
                /** This PTE is accessed by hardware or not */
                access : 1,
                /** This PTE is written by hardware or not */
                dirty : 1, _unused : 4;
};

/**
 * @brief Parse an architecture-specific to a common PTE.
 *
 * @param pte [In] pointer to an architecture-specific PTE
 * @param level [In] The level in page table where the @pte reside. WARNING:
 * Currently, the last level PTE is encoded as level 3, and the levels above
 * it are encoded as level 2, 1, 0(if the architecture has 4-level page table,
 * otherwise the smallest level is 1). But levels except 3 is not used in this
 * function now. And if we would implement 5-level page tables in the future,
 * we should change the encoding of levels.
 * @param ret [Out] returning the parsed common PTE
 */
void parse_pte_to_common(pte_t *pte, unsigned int level,
                         struct common_pte_t *ret);
/**
 * @brief Assign settings in the common PTE to an architecture-specific PTE.
 *
 * @param dest [In] pointer to an architecture-specific PTE
 * @param level [In] The level in page table where the @dest reside. WARNING:
 * Currently, the last level PTE is encoded as level 3, and the levels above
 * it are encoded as level 2, 1, 0(if the architecture has 4-level page table,
 * otherwise the smallest level is 1). But levels except 3 is not used in this
 * function now. And if we would implement 5-level page tables in the future,
 * we should change the encoding of levels.
 * @param src [In] pointer to the common PTE
 */
void update_pte(pte_t *dest, unsigned int level, struct common_pte_t *src);

#endif /* MM_COMMON_PTE_H */