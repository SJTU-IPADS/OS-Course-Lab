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
 * Region header file.
 */
#pragma once

#include "g2d_internal.h"

PRGN _create_rgn(int size);
int _set_rc_rgn(PRGN rgn, int l, int t, int r, int b);
int _set_dc_rgn(PRGN rgn, const PDC dc);
void _del_rgn(PRGN rgn);
RECT *_get_rgn_rc(PRGN rgn, enum GET_RGN_RC_CMD cmd);
int _combine_rgn(PRGN dst, PRGN src1, PRGN src2, enum RGN_COMB_MODE mode);
void _clear_rgn(PRGN rgn);
void _move_rgn(PRGN rgn, int x, int y);
bool _pt_in_rgn(const PRGN rgn, int x, int y);
bool _rgn_null(const PRGN rgn);
void _print_rgn(const PRGN rgn);

u32 rgn_rc_num(const PRGN rgn);
const RECT *rgn_rc(const PRGN rgn, u32 i);

/* Local regions */
void init_gdi_rgn_ex(void);
PRGN get_gdi_rgn_ex0(void);
PRGN get_gdi_rgn_ex1(void);
