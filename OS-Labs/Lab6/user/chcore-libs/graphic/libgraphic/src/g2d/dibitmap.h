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
 * Device-independent bitmap header file.
 */
#pragma once

#include "g2d_internal.h"

/* Error codes */
enum DIB_STATUS {
        DIB_OK = 0, /* No error */
        DIB_ERR, /* General error */
        DIB_OUT_OF_MEMORY, /* Could not allocate enough memory to complete the
                              operation */
        DIB_IO_ERR, /* General input/output error */
        DIB_FILE_NOT_FOUND, /* File not found */
        DIB_FILE_NOT_SUPPORTED, /* File is not a supported DIB variant */
        DIB_FILE_INVALID, /* File is not a DIB image or is an invalid DIB */
        DIB_INVALID_ARG, /* An argument is invalid or out of range */
        DIB_TYPE_MISMATCH, /* The requested action is not compatible with the
                              DIB's type */
        DIB_ERR_MAX
};

/* Construction/destruction */
PDIB _create_dib(u32 w, u32 h, u32 depth);
void _del_dib(PDIB dib);

/* I/O */
PDIB _load_dib(const char *filename);
void _save_dib(const PDIB dib, const char *filename);

/* Meta info */
u32 _dib_get_width(const PDIB dib);
u32 _dib_get_height(const PDIB dib);
u16 _dib_get_depth(const PDIB dib);

/* Pixel access */
void _dib_get_pixel_rgb(const PDIB dib, u32 x, u32 y, u8 *r, u8 *g, u8 *b);
void _dib_set_pixel_rgb(PDIB dib, u32 x, u32 y, u8 r, u8 g, u8 b);
void _dib_get_pixel_index(const PDIB dib, u32 x, u32 y, u8 *val);
void _dib_set_pixel_index(PDIB dib, u32 x, u32 y, u8 val);

/* Palette handling */
void _dib_get_palette_color(const PDIB dib, u8 index, u8 *r, u8 *g, u8 *b);
void _dib_set_palette_color(PDIB dib, u8 index, u8 r, u8 g, u8 b);

/* Error handling */
enum DIB_STATUS _dib_get_error(void);
const char *_dib_get_error_desc(void);

int dib_width(PDIB dib);
int dib_height(PDIB dib);
int dib_depth(PDIB dib);
u8 *dib_data(PDIB dib);
