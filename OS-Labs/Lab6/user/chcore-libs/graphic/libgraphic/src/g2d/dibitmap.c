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
 * Implementation of device-independent bitmap.
 */
#include "dibitmap.h"

#include "dc.h"
#include "../dep/os.h"

/* Size of the palette data for 8 BPP dib */
#define DIB_PALETTE_SIZE_8BPP (256 * 4)

/* Size of the palette data for 4 BPP dib */
#define DIB_PALETTE_SIZE_4BPP (16 * 4)

/* DIBitmap header */
struct dib_header {
        u16 magic; /* Magic identifier: "BM" */
        u32 file_size; /* Size of the DIB file in bytes */
        u16 reserved1; /* Reserved */
        u16 reserved2; /* Reserved */
        u32 data_offset; /* Offset of image data relative to the file's start */
        u32 header_size; /* Size of the header in bytes */
        u32 w; /* DIB's width */
        u32 h; /* DIB's height */
        u16 planes; /* Number of color planes in the DIB */
        u16 bpp; /* Number of bits per pixel */
        u32 compression_type; /* Compression type */
        u32 img_data_size; /* Size of uncompressed image's data */
        u32 hpixels_per_meter; /* Horizontal resolution (pixels per meter) */
        u32 vpixels_per_meter; /* Vertical resolution (pixels per meter) */
        u32 colors_used; /* Number of color indexes in the color table that are
                            actually used by the DIB */
        u32 colors_required; /* Number of color indexes that are required for
                                displaying the DIB */
};

struct dibitmap {
        struct dib_header header;
        u8 *palette;
        u8 *data;
};

/* The last error code */
static enum DIB_STATUS dib_last_err_code = DIB_OK;

/* Error description strings */
static const char *dib_err_desc_strs[] = {
        "",
        "General error",
        "Could not allocate enough memory to complete the operation",
        "File input/output error",
        "File not found",
        "File is not a supported DIB variant (must be uncompressed 4, 8, 24 or 32 BPP)",
        "File is not a valid DIB image",
        "An argument is invalid or out of range",
        "The requested action is not compatible with the DIB's type"};

/* Return 1 if succ */
static int read_u32(u32 *x, int fd)
{
        u8 little[4];
        if (read(fd, little, 4) != 4)
                return 0;
        *x = (little[3] << 24 | little[2] << 16 | little[1] << 8 | little[0]);
        return 1;
}

/* Return 1 if succ */
static int read_u16(u16 *x, int fd)
{
        u8 little[2];
        if (read(fd, little, 2) != 2)
                return 0;
        *x = (little[1] << 8 | little[0]);
        return 1;
}

/* Return 1 if succ */
static int write_u32(u32 x, int fd)
{
        u8 little[4];

        little[3] = (u8)((x & 0xff000000) >> 24);
        little[2] = (u8)((x & 0x00ff0000) >> 16);
        little[1] = (u8)((x & 0x0000ff00) >> 8);
        little[0] = (u8)((x & 0x000000ff) >> 0);

        return write(fd, little, 4) == 4;
}

/* Return 1 if succ */
static int write_u16(u16 x, int fd)
{
        u8 little[2];

        little[1] = (u8)((x & 0xff00) >> 8);
        little[0] = (u8)((x & 0x00ff) >> 0);

        return write(fd, little, 2) == 2;
};

/*
 * Read the DIB file's header into the data structure.
 * Return DIB_OK if succ.
 */
static int dib_read_header(PDIB dib, int fd)
{
        /*
         * Read the header's fields one by one,
         * and convert the format's little endian
         * to system's native representation.
         */
        if (!read_u16(&(dib->header.magic), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.file_size), fd))
                return DIB_IO_ERR;
        if (!read_u16(&(dib->header.reserved1), fd))
                return DIB_IO_ERR;
        if (!read_u16(&(dib->header.reserved2), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.data_offset), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.header_size), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.w), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.h), fd))
                return DIB_IO_ERR;
        if (!read_u16(&(dib->header.planes), fd))
                return DIB_IO_ERR;
        if (!read_u16(&(dib->header.bpp), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.compression_type), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.img_data_size), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.hpixels_per_meter), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.vpixels_per_meter), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.colors_used), fd))
                return DIB_IO_ERR;
        if (!read_u32(&(dib->header.colors_required), fd))
                return DIB_IO_ERR;

        return DIB_OK;
}

/*
 * Write the DIB file's header into the data structure.
 * Return DIB_OK if succ.
 */
static int dib_write_header(const PDIB dib, int fd)
{
        /*
         * Write the header's fields one by one,
         * and convert to the format's little endian representation.
         */
        if (!write_u16(dib->header.magic, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.file_size, fd))
                return DIB_IO_ERR;
        if (!write_u16(dib->header.reserved1, fd))
                return DIB_IO_ERR;
        if (!write_u16(dib->header.reserved2, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.data_offset, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.header_size, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.w, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.h, fd))
                return DIB_IO_ERR;
        if (!write_u16(dib->header.planes, fd))
                return DIB_IO_ERR;
        if (!write_u16(dib->header.bpp, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.compression_type, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.img_data_size, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.hpixels_per_meter, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.vpixels_per_meter, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.colors_used, fd))
                return DIB_IO_ERR;
        if (!write_u32(dib->header.colors_required, fd))
                return DIB_IO_ERR;

        return DIB_OK;
}

/*
 * Create a blank DIB image with the specified dimensions and bit depth.
 */
PDIB _create_dib(u32 w, u32 h, u32 depth)
{
        PDIB dib;
        u32 bits_per_row;
        u32 palette_size = 0;

        if (depth == 8)
                palette_size = DIB_PALETTE_SIZE_8BPP;
        else if (depth == 4)
                palette_size = DIB_PALETTE_SIZE_4BPP;

        if (w <= 0 || h <= 0) {
                dib_last_err_code = DIB_INVALID_ARG;
                return NULL;
        }
        if (depth != 4 && depth != 8 && depth != 24 && depth != 32) {
                dib_last_err_code = DIB_FILE_NOT_SUPPORTED;
                return NULL;
        }

        /* Allocate the bitmap data structure */
        dib = (PDIB)calloc(1, sizeof(*dib));
        if (dib == NULL) {
                dib_last_err_code = DIB_OUT_OF_MEMORY;
                return NULL;
        }

        /* Set header's default values */
        dib->header.magic = 0x4D42;
        dib->header.reserved1 = 0;
        dib->header.reserved2 = 0;
        dib->header.header_size = 40;
        dib->header.planes = 1;
        dib->header.compression_type = 0;
        dib->header.hpixels_per_meter = 0;
        dib->header.vpixels_per_meter = 0;
        dib->header.colors_used = 0;
        dib->header.colors_required = 0;

        /*
         * Calculate the number of bits used to store a single image row.
         * This is always rounded up to the next multiple of 32 (4 bytes).
         */
        bits_per_row = w * depth;
        bits_per_row += (bits_per_row % 32 ? 32 - bits_per_row % 32 : 0);

        /* Set header's image specific values */
        dib->header.w = w;
        dib->header.h = h;
        dib->header.bpp = depth;
        dib->header.img_data_size = h * (bits_per_row >> 3);
        dib->header.file_size = 54 + palette_size + dib->header.img_data_size;
        dib->header.data_offset = 54 + palette_size;

        /* Allocate palette */
        if (dib->header.bpp == 8 || dib->header.bpp == 4) {
                dib->palette = (u8 *)calloc(palette_size, sizeof(u8));
                if (dib->palette == NULL) {
                        dib_last_err_code = DIB_OUT_OF_MEMORY;
                        free(dib);
                        return NULL;
                }
        } else {
                dib->palette = NULL;
        }

        /* Allocate pixels */
        dib->data = (u8 *)calloc(dib->header.img_data_size, sizeof(u8));
        if (dib->data == NULL) {
                dib_last_err_code = DIB_OUT_OF_MEMORY;
                free(dib->palette);
                free(dib);
                return NULL;
        }

        dib_last_err_code = DIB_OK;
        return dib;
}

/*
 * Free all the memory used by the specified DIB image.
 */
void _del_dib(PDIB dib)
{
        if (dib == NULL)
                return;
        if (dib->palette != NULL)
                free(dib->palette);
        if (dib->data != NULL)
                free(dib->data);
        free(dib);
        dib_last_err_code = DIB_OK;
}

/*
 * Read the specified DIB image file.
 */
PDIB _load_dib(const char *filename)
{
        PDIB dib;
        int fd;
        u32 palette_size = 0;
        int fread_res;

        if (filename == NULL) {
                dib_last_err_code = DIB_INVALID_ARG;
                return NULL;
        }

        /* Open file */
        fd = open(filename, 0);
        if (fd == 0) {
                dib_last_err_code = DIB_FILE_NOT_FOUND;
                return NULL;
        }

        /* Allocate */
        dib = (PDIB)calloc(1, sizeof(*dib));
        if (dib == NULL) {
                dib_last_err_code = DIB_OUT_OF_MEMORY;
                return NULL;
        }

        /* Read header */
        if (dib_read_header(dib, fd) != DIB_OK || dib->header.magic != 0x4D42) {
                dib_last_err_code = DIB_FILE_INVALID;
                close(fd);
                free(dib);
                return NULL;
        }

        /* img_data_size may be 0 */
        if (dib->header.img_data_size == 0) {
                dib->header.img_data_size =
                        dib->header.w * dib->header.h * (dib->header.bpp >> 3);
        }

        if (dib->header.bpp == 8)
                palette_size = DIB_PALETTE_SIZE_8BPP;
        else if (dib->header.bpp == 4)
                palette_size = DIB_PALETTE_SIZE_4BPP;

        /* Verify that the bitmap variant is supported */
        if ((dib->header.bpp != 32 && dib->header.bpp != 24
             && dib->header.bpp != 8 && dib->header.bpp != 4)
            || dib->header.compression_type != 0
            || dib->header.header_size != 40) {
                dib_last_err_code = DIB_FILE_NOT_SUPPORTED;
                close(fd);
                free(dib);
                return NULL;
        }

        /* Allocate and read palette */
        if (palette_size > 0) {
                dib->palette = (u8 *)malloc(palette_size * sizeof(u8));
                if (dib->palette == NULL) {
                        dib_last_err_code = DIB_OUT_OF_MEMORY;
                        close(fd);
                        free(dib);
                        return NULL;
                }

                if (read(fd, dib->palette, palette_size * sizeof(u8))
                    != palette_size) {
                        dib_last_err_code = DIB_FILE_INVALID;
                        close(fd);
                        free(dib->palette);
                        free(dib);
                        return NULL;
                }
        }
        /* Not an indexed image */
        else {
                dib->palette = NULL;
        }

        /* Allocate memory for image data */
        dib->data = (u8 *)malloc(dib->header.img_data_size);
        if (dib->data == NULL) {
                dib_last_err_code = DIB_OUT_OF_MEMORY;
                close(fd);
                free(dib->palette);
                free(dib);
                return NULL;
        }

        /* Read image data */
        if ((fread_res = read(fd, dib->data, dib->header.img_data_size))
            != dib->header.img_data_size) {
                dib_last_err_code = DIB_FILE_INVALID;
                close(fd);
                free(dib->data);
                free(dib->palette);
                free(dib);
                return NULL;
        }

        close(fd);
        dib_last_err_code = DIB_OK;
        return dib;
}

/*
 * Write the DIB image to the specified file.
 */
void _save_dib(PDIB dib, const char *filename)
{
        int fd;
        u32 palette_size = 0;

        if (dib->header.bpp == 8)
                palette_size = DIB_PALETTE_SIZE_8BPP;
        else if (dib->header.bpp == 4)
                palette_size = DIB_PALETTE_SIZE_4BPP;

        if (filename == NULL) {
                dib_last_err_code = DIB_INVALID_ARG;
                return;
        }

        /* Open file */
        fd = open(filename, O_CREAT);
        if (fd == 0) {
                dib_last_err_code = DIB_FILE_NOT_FOUND;
                return;
        }

        /* Write header */
        if (dib_write_header(dib, fd) != DIB_OK) {
                dib_last_err_code = DIB_IO_ERR;
                close(fd);
                return;
        }

        /* Write palette */
        if (palette_size > 0) {
                if (write(fd, dib->palette, palette_size * sizeof(u8))
                    != palette_size * sizeof(u8)) {
                        dib_last_err_code = DIB_IO_ERR;
                        close(fd);
                        return;
                }
        }

        /* Write data */
        if (write(fd, dib->data, dib->header.img_data_size * sizeof(u8))
            != dib->header.img_data_size * sizeof(u8)) {
                dib_last_err_code = DIB_IO_ERR;
                close(fd);
                return;
        }

        dib_last_err_code = DIB_OK;
        close(fd);
}

/*
 * Return the DIB image's width.
 */
inline u32 _dib_get_width(const PDIB dib)
{
        if (dib == NULL) {
                dib_last_err_code = DIB_INVALID_ARG;
                return 0;
        }
        dib_last_err_code = DIB_OK;
        return dib->header.w;
}

/*
 * Return the DIB image's height.
 */
inline u32 _dib_get_height(const PDIB dib)
{
        if (dib == NULL) {
                dib_last_err_code = DIB_INVALID_ARG;
                return 0;
        }
        dib_last_err_code = DIB_OK;
        return dib->header.h;
}

/*
 * Returns the DIB image's color depth (bits per pixel).
 */
inline u16 _dib_get_depth(const PDIB dib)
{
        if (dib == NULL) {
                dib_last_err_code = DIB_INVALID_ARG;
                return 0;
        }
        dib_last_err_code = DIB_OK;
        return dib->header.bpp;
}

/*
 * Populate the arguments with the specified pixel's RGB values.
 */
void _dib_get_pixel_rgb(const PDIB dib, u32 x, u32 y, u8 *r, u8 *g, u8 *b)
{
        u8 *pixel;
        u32 bytes_per_row;
        u8 bytes_per_pixel;

        if (dib == NULL || x < 0 || x >= dib->header.w || y < 0
            || y >= dib->header.h)
                dib_last_err_code = DIB_INVALID_ARG;
        else {
                dib_last_err_code = DIB_OK;
                bytes_per_pixel = dib->header.bpp >> 3;
                /* Row's size is rounded up to the next multiple of 4 bytes */
                bytes_per_row = dib->header.img_data_size / dib->header.h;
                /* Calculate the location of the relevant pixel (rows are
                 * flipped) */
                pixel = dib->data
                        + ((dib->header.h - y - 1) * bytes_per_row
                           + x * bytes_per_pixel);
                /*
                 * In indexed color mode the pixel's value is an index
                 * within the palette.
                 */
                if (dib->palette != NULL) {
                        pixel = dib->palette + *pixel * 4; // TODO - por para
                                                           // 4bpp
                }
                /* Note: colors are stored in BGR order */
                if (r)
                        *r = *(pixel + 2);
                if (g)
                        *g = *(pixel + 1);
                if (b)
                        *b = *(pixel + 0);
        }
}

/*
 * Set the specified pixel's RGB values.
 */
void _dib_set_pixel_rgb(PDIB dib, u32 x, u32 y, u8 r, u8 g, u8 b)
{
        u8 *pixel;
        u32 bytes_per_row;
        u8 bytes_per_pixel;

        if (dib == NULL || x < 0 || x >= dib->header.w || y < 0
            || y >= dib->header.h)
                dib_last_err_code = DIB_INVALID_ARG;
        else if (dib->palette != NULL)
                dib_last_err_code = DIB_TYPE_MISMATCH;
        else {
                dib_last_err_code = DIB_OK;
                bytes_per_pixel = dib->header.bpp >> 3;
                /* Row's size is rounded up to the next multiple of 4 bytes */
                bytes_per_row = dib->header.img_data_size / dib->header.h;
                /* Calculate the location of the relevant pixel (rows are
                 * flipped) */
                pixel = dib->data
                        + ((dib->header.h - y - 1) * bytes_per_row
                           + x * bytes_per_pixel);
                /* Note: colors are stored in BGR order */
                *(pixel + 2) = r;
                *(pixel + 1) = g;
                *(pixel + 0) = b;
        }
}

/*
 * Get the specified pixel's color index.
 */
void _dib_get_pixel_index(const PDIB dib, u32 x, u32 y, u8 *val)
{
        u8 *pixel;
        u32 bytes_per_row;
        u32 tx;

        if (dib == NULL || val == NULL || x < 0 || x >= dib->header.w || y < 0
            || y >= dib->header.h)
                dib_last_err_code = DIB_INVALID_ARG;
        else if (dib->palette == NULL)
                dib_last_err_code = DIB_TYPE_MISMATCH;
        else {
                dib_last_err_code = DIB_OK;
                /* Row's size is rounded up to the next multiple of 4 bytes */
                bytes_per_row = dib->header.img_data_size / dib->header.h;
                /* Calculate the location of the relevant pixel for 8bpp */
                if (dib->header.bpp == 8) {
                        pixel = dib->data
                                + ((dib->header.h - y - 1) * bytes_per_row + x);
                        *val = *pixel;
                }
                /* Calculate the location of the relevant pixel for 4bpp */
                else {
                        /* Divide x by two and round to floor */
                        tx = (x % 2 == 0 ? x / 2 : (x - 1) / 2);
                        pixel = dib->data
                                + ((dib->header.h - y - 1) * bytes_per_row
                                   + tx);
                        if (x % 2 == 0)
                                *val = ((*pixel & 0xF0) >> 4);
                        else
                                *val = *pixel & 0x0F;
                }
        }
}

/*
 * Sets the specified pixel's color index.
 */
void _dib_set_pixel_index(PDIB dib, u32 x, u32 y, u8 val)
{
        u8 *pixel;
        u32 bytes_per_row;
        u32 tx;

        if (dib == NULL || x >= dib->header.w || y >= dib->header.h
            || (dib->header.bpp == 4 && val >= 16))
                dib_last_err_code = DIB_INVALID_ARG;
        else if (dib->palette == NULL)
                dib_last_err_code = DIB_TYPE_MISMATCH;
        else {
                dib_last_err_code = DIB_OK;
                /* Row's size is rounded up to the next multiple of 4 bytes */
                bytes_per_row = dib->header.img_data_size / dib->header.h;
                /* Calculate the location of the relevant pixel for 8bpp */
                if (dib->header.bpp == 8) {
                        pixel = dib->data
                                + ((dib->header.h - y - 1) * bytes_per_row + x);
                        *pixel = val;
                }
                /* Calculate the location of the relevant pixel for 4bpp */
                else {
                        /* Divide x by two and round to floor */
                        tx = (x % 2 == 0 ? x / 2 : (x - 1) / 2);
                        pixel = dib->data
                                + ((dib->header.h - y - 1) * bytes_per_row
                                   + tx);
                        /* Put the 4bit value in *pixel */
                        if (x % 2 == 0) {
                                *pixel &= 0x0F; // clear bits
                                *pixel |= (val << 4); // set bits
                        } else {
                                *pixel &= 0xF0; // clear bits
                                *pixel |= val; // set bits
                        }
                }
        }
}

/*
 * Get the color value for the specified palette index.
 */
void _dib_get_palette_color(const PDIB dib, u8 index, u8 *r, u8 *g, u8 *b)
{
        if (dib == NULL || (dib->header.bpp == 4 && index >= 16))
                dib_last_err_code = DIB_INVALID_ARG;
        else if (dib->palette == NULL)
                dib_last_err_code = DIB_TYPE_MISMATCH;
        else {
                /* Note: colors are stored in BGR order */
                if (r)
                        *r = *(dib->palette + (index << 2) + 2);
                if (g)
                        *g = *(dib->palette + (index << 2) + 1);
                if (b)
                        *b = *(dib->palette + (index << 2) + 0);
                dib_last_err_code = DIB_OK;
        }
}

/*
 * Set the color value for the specified palette index.
 */
void _dib_set_palette_color(PDIB dib, u8 index, u8 r, u8 g, u8 b)
{
        if (dib == NULL || (dib->header.bpp == 4 && index >= 16))
                dib_last_err_code = DIB_INVALID_ARG;
        else if (dib->palette == NULL)
                dib_last_err_code = DIB_TYPE_MISMATCH;
        else {
                /* Note: colors are stored in BGR order */
                *(dib->palette + (index << 2) + 2) = r;
                *(dib->palette + (index << 2) + 1) = g;
                *(dib->palette + (index << 2) + 0) = b;
                dib_last_err_code = DIB_OK;
        }
}

/*
 * Return the last error code.
 */
inline enum DIB_STATUS _dib_get_error(void)
{
        return dib_last_err_code;
}

/*
 * Return a description of the last error code.
 */
inline const char *_dib_get_error_desc(void)
{
        if (dib_last_err_code > 0 && dib_last_err_code < DIB_ERR_MAX)
                return dib_err_desc_strs[dib_last_err_code];
        else
                return NULL;
}

inline int dib_width(PDIB dib)
{
        return dib->header.w;
}

inline int dib_height(PDIB dib)
{
        return dib->header.h;
}

inline int dib_depth(PDIB dib)
{
        return dib->header.bpp;
}

inline u8 *dib_data(PDIB dib)
{
        return dib->data;
}
