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
 * GDI engine header file.
 * Define basic types, structs and functions of GDI engine.
 */
#pragma once

#include "g2d_base.h"

/* Device Context Type */
enum DC_TYPE {
        DC_NULL, /* NULL DC, seldom used */
        DC_DEVICE, /* Device DC, used by screen */
        DC_OFFSCREEN, /* Offscreen DC, used by GUI clients */
        DC_MEM /* Memory DC, for common use */
};

/* Get Region Rectangle Command */
enum GET_RGN_RC_CMD {
        GRR_FIRST, /* Get the first rect of the region */
        GRR_LAST, /* Get the last rect of the region */
        GRR_NEXT, /* Get the next rect of the region */
        GRR_PREV /* Get the prev rect of the region */
};

/* Region Combination Mode */
enum RGN_COMB_MODE {
        RGN_ADD, /* dst = src1 + src2 */
        RGN_AND, /* dst = src1 & src2 */
        RGN_COPY, /* dst = src */
        RGN_DIFF, /* dst = src1 - src2 */
        RGN_OR, /* dst = src1 | src2 */
        RGN_XOR /* dst = src1 ^ src2 */
};

/*
 * Raster Operation Type.
 * We only support ROP_SRCAND, ROP_SRCCOPY, ROP_SRCERASE,
 * ROP_SRCINVERT, ROP_SRCPAINT now.
 */
enum ROP {
        ROP_BLACKNESS,
        ROP_CAPTUREBLT,
        ROP_DSTINVERT,
        ROP_MERGECOPY,
        ROP_MERGEPAINT,
        ROP_NOMIRRORBITMAP,
        ROP_NOTSRCCOPY,
        ROP_NOTSRCERASE,
        ROP_PATCOPY,
        ROP_PATINVERT,
        ROP_PATPAINT,
        ROP_SRCAND, /* dst &= src */
        ROP_SRCCOPY, /* dst = src */
        ROP_SRCERASE, /* dst = ~src; dst &= src */
        ROP_SRCINVERT, /* dst ^= src*/
        ROP_SRCPAINT, /* dst |= src */
        ROP_WHITENESS
};

/* Orientation type */
enum ORIENTATION {
        ORI_NONE,
        LEFT,
        TOP,
        RIGHT,
        BOTTOM,
        TOP_LEFT,
        BOTTOM_LEFT,
        TOP_RIGHT,
        BOTTOM_RIGHT
};

/* Commont pointer */
typedef void *POINTER;
/* Pointer of bitmap */
typedef struct bitmap *PBITMAP;
/* Pointer of brush */
typedef struct brush *PBRUSH;
/* Pointer of device context */
typedef struct device_context *PDC;
/* Pointer of device-independent bitmap */
typedef struct dibitmap *PDIB;
/* Pointer of font */
typedef struct font *PFONT;
/* Pointer of region */
typedef struct region *PRGN;

/*
 * Arc
 */

/**
 * Draw an arc.
 * @param dc Pointer to the device context where drawing takes place.
 * @param x1 The x-coordinate, in pixels, of the left bounder of the ellipse.
 * @param y1 The y-coordinate, in pixels, of the top bounder of the ellipse.
 * @param x2 The x-coordinate, in pixels, of the right bounder of the ellipse.
 * @param y2 The y-coordinate, in pixels, of the bottom bounder of the ellipse.
 * @param x3 The x-coordinate, in pixels, of the ending point of the radial line
 * 	defining the starting point of the arc.
 * @param y3 The y-coordinate, in pixels, of the ending point of the radial line
 * 	defining the starting point of the arc.
 * @param x4 The x-coordinate, in pixels, of the ending point of the radial line
 * 	defining the ending point of the arc.
 * @param y4 The y-coordinate, in pixels, of the ending point of the radial line
 * 	defining the ending point of the arc.
 * @return 0 if succ.
 */
int draw_arc(const PDC dc, int x1, int y1, int x2, int y2, int x3, int y3,
             int x4, int y4);
/**
 * Draw an ellipse.
 * @param dc Pointer to the device context where drawing takes place.
 * @param left The x-coordinate, in pixels, of the left bounder of the ellipse.
 * @param top The y-coordinate, in pixels, of the top bounder of the ellipse.
 * @param right The x-coordinate, in pixels, of the right bounder of the
 * ellipse.
 * @param bottom The y-coordinate, in pixels, of the bottom bounder of the
 * ellipse.
 * @return 0 if succ.
 */
int draw_ellipse(const PDC dc, int left, int top, int right, int bottom);

/*
 * Bitmap
 */

/**
 * Create a bitmap.
 * @param width Pixel width of bitmap.
 * @param height Pixel height of bitmap.
 * @param depth Color depth of bitmap (usually 32, 24, 16, etc).
 * @param pitch Pitch of bitmap (usually == width * height * (depth / 8))
 * @return Pointer to created bitmap
 */
PBITMAP create_bitmap(int width, int height, int depth, int pitch);
/**
 * Create a bitmap from a pmo_cap.
 * @param width Pixel width of bitmap.
 * @param height Pixel height of bitmap.
 * @param depth Color depth of bitmap (usually 32, 24, 16, etc).
 * @param pitch Pitch of bitmap (usually == width * height * (depth / 8))
 * @param pmo_cap Cap of the pmo.
 * @return Pointer to created bitmap
 */
PBITMAP create_bitmap_from_pmo(int width, int height, int depth, int pitch,
                               int pmo_cap);
/**
 * Create a bitmap from specified virtual address.
 * @param width Pixel width of bitmap.
 * @param height Pixel height of bitmap.
 * @param depth Color depth of bitmap (usually 32, 24, 16, etc).
 * @param pitch Pitch of bitmap (usually == width * height * (depth / 8))
 * @param vaddr Virtual address of the bitmap's real bits.
 * @return Pointer to created bitmap
 */
PBITMAP create_bitmap_from_vaddr(int width, int height, int depth, int pitch,
                                 void *vaddr);
/**
 * Create a bitmap compatible with the specified device context.
 * @param dc Pointer to the device context.
 * @param width Pixel width of bitmap.
 * @param height Pixel height of bitmap.
 * @return Pointer to created bitmap
 */
PBITMAP create_compat_bitmap(const PDC dc, int width, int height);
/**
 * Delete the bitmap.
 * @param bitmap Pointer to the bitmap.
 */
void delete_bitmap(PBITMAP bitmap);
/**
 * Clear the bitmap to ZERO.
 * @param bitmap Pointer to the bitmap.
 */
void clear_bitmap(PBITMAP bitmap);
/**
 * Get pmo_cap of the bitmap, if any.
 * @param bitmap Pointer to the bitmap.
 * @return pmo_cap of the bitmap.
 */
int get_bitmap_pmo_cap(const PBITMAP bitmap);
/**
 * Get width of the bitmap.
 * @param bitmap Pointer to the bitmap.
 * @return height of the bitmap.
 */
int get_bitmap_width(const PBITMAP bitmap);
/**
 * Get height of the bitmap.
 * @param bitmap Pointer to the bitmap.
 * @return height of the bitmap.
 */
int get_bitmap_height(const PBITMAP bitmap);
/**
 * Get depth of the bitmap.
 * @param bitmap Pointer to the bitmap.
 * @return depth of the bitmap.
 */
int get_bitmap_depth(const PBITMAP bitmap);
/**
 * Get pitch of the bitmap.
 * @param bitmap Pointer to the bitmap.
 * @return pitch of the bitmap.
 */
int get_bitmap_pitch(const PBITMAP bitmap);
/**
 * Bit-Block transfer. Copy one rect area from src DC to dst DC.
 * @param dst Pointer to the destination device context.
 * @param x1 The x-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param y1 The y-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param width The width, in pixels, of the source and destination rects.
 * @param height The height, in pixels, of the source and the destination rects.
 * @param src Pointer to the source device context.
 * @param x2 The x-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param y2 The y-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param rop The raster-operation code, which defines how the color data of the
 * source rect is to be combined with the color data of the destination rect to
 * achieve the final color.
 * @return 0 if succ.
 */
int bitblt(const PDC dst, int x1, int y1, int width, int height, const PDC src,
           int x2, int y2, enum ROP rop);
/**
 * Stretch or compress the source rect to fit the destination rect.
 * @param dst Pointer to the destination device context.
 * @param x1 The x-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param y1 The y-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param w1 The width, in pixels, of the destination rect.
 * @param h1 The height, in pixels, of the destination rect.
 * @param src Pointer to the source device context.
 * @param x2 The x-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param y2 The y-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param w2 The width, in pixels, of the source rect.
 * @param h2 The height, in pixels, of the source rect.
 * @param rop The raster-operation code, which defines how the color data of the
 * source rect is to be combined with the color data of the destination rect to
 * achieve the final color.
 * @return 0 if succ.
 */
int stretch_blt(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
                int x2, int y2, int w2, int h2, enum ROP rop);
/**
 * Stretch or compress the rect of the bitmap to fit the destination rect.
 * @param dst Pointer to the destination device context.
 * @param x1 The x-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param y1 The y-coordinate, in pixels, of the upper-left corner of the
 * destination rect.
 * @param w1 The width, in pixels, of the destination rect.
 * @param h1 The height, in pixels, of the destination rect.
 * @param bitmap Pointer to the source bitmap.
 * @param x2 The x-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param y2 The y-coordinate, in pixels, of the upper-left corner of the source
 * rect.
 * @param w2 The width, in pixels, of the source rect.
 * @param h2 The height, in pixels, of the source rect.
 * @param rop The raster-operation code, which defines how the color data of the
 * source rect is to be combined with the color data of the destination rect to
 * achieve the final color.
 * @return 0 if succ.
 */
int stretch_bitmap(const PDC dst, int x1, int y1, int w1, int h1,
                   const PBITMAP bitmap, int x2, int y2, int w2, int h2,
                   enum ROP rop);
int transparent_blt(const PDC dst, int x1, int y1, int w1, int h1,
                    const PDC src, int x2, int y2, int w2, int h2, u32 color);
/**
 * Create a compatible bitmap from a device-independent bitmap.
 * @param dc Pointer to the device context.
 * @param dib Pointer to the device-independent bitmap.
 * @return Pointer to the created bitmap.
 */
PBITMAP create_dib_bitmap(const PDC dc, const PDIB dib);

/*
 * Blend
 */

int blend(const PDC dst, int x1, int y1, int w1, int h1, const PDC src, int x2,
          int y2, int w2, int h2, u8 alpha);
int blend_rgba(const PDC dst, int x1, int y1, int w1, int h1, const PDC src,
               int x2, int y2, int w2, int h2, u8 alpha);

/*
 * Brush
 */

/**
 * Create a brush. Useless now.
 * @return Pointer to the created brush.
 */
PBRUSH create_brush(void);
/**
 * Delete the specified brush.
 * @param brush Pointer to the brush to be deleted.
 */
void delete_brush(PBRUSH brush);

/*
 * Device Context
 */

/**
 * Create a device context with the specified type.
 * @param type Type of the device context to be created.
 * @return Pointer to the created device context.
 */
PDC create_dc(enum DC_TYPE type);
/**
 * Delete the specified device context.
 * @param dc Pointer to the device context to be deleted.
 */
void delete_dc(PDC dc);
/**
 * Select the bitmap for the device context.
 * @param dc Pointer to the device context.
 * @param bitmap Pointer to the bitmap.
 */
void dc_select_bitmap(PDC dc, const PBITMAP bitmap);
/**
 * Select the clip region for the device context.
 * @param dc Pointer to the device context.
 * @param rgn Pointer to the clip region.
 */
void dc_select_clip_rgn(PDC dc, PRGN rgn);
/**
 * Select the font for the device context.
 * @param dc Pointer to the device context.
 * @param font Pointer to the font.
 */
void dc_select_font(PDC dc, PFONT font);
/**
 * Release the clip region of the device context.
 * @param dc Pointer to the device context.
 */
void dc_release_clip_rgn(PDC dc);
/**
 * Get the pmo_cap of the device context.
 * @param dc Pointer to the device context.
 * @return pmo_cap of the device context
 */
int get_dc_pmo_cap(const PDC dc);
/**
 * Get the fill brush of the device context.
 * @param dc Pointer to the device context.
 * @return Pointer to the fill brush of the device context.
 */
PBRUSH get_dc_fill_brush(PDC dc);
/**
 * Get the paint brush of the device context.
 * @param dc Pointer to the device context.
 * @return Pointer to the paint brush of the device context.
 */
PBRUSH get_dc_paint_brush(PDC dc);
/**
 * Get the text brush of the device context.
 * @param dc Pointer to the device context.
 * @return Pointer to the text brush of the device context.
 */
PBRUSH get_dc_txt_brush(PDC dc);
/**
 * Get the background brush of the device context.
 * @param dc Pointer to the device context.
 * @return Pointer to the background brush of the device context.
 */
PBRUSH get_dc_bg_brush(PDC dc);
/**
 * Get the pixel width of the device context.
 * @param dc Pointer to the device context.
 * @return Width ot the device context.
 */
int get_dc_width(const PDC dc);
/**
 * Get the pixel height of the device context.
 * @param dc Pointer to the device context.
 * @return Height of the device context.
 */
int get_dc_height(const PDC dc);
/**
 * Get the bit width of the font of the device context.
 * @param dc Pointer to the device context.
 * @return Bit width of the font of the device context.
 */
int get_dc_font_bitwidth(const PDC dc);
/**
 * Get the bit height of the font of the device context.
 * @param dc Pointer to the device context.
 * @return Bit height of the font of the device context.
 */
int get_dc_font_bitheight(const PDC dc);
/**
 * Set the fill brush of the device context.
 * @param dc Pointer to the device context.
 * @param brush Pointer to the fill brush.
 */
void set_dc_fill_brush(PDC dc, PBRUSH brush);
/**
 * Set the paint brush of the device context.
 * @param dc Pointer to the device context.
 * @param brush Pointer to the paint brush.
 */
void set_dc_paint_brush(PDC dc, PBRUSH brush);
/**
 * Set the text brush of the device context.
 * @param dc Pointer to the device context.
 * @param brush Pointer to the text brush.
 */
void set_dc_txt_brush(PDC dc, PBRUSH brush);
/**
 * Set the background brush of the device context.
 * @param dc Pointer to the device context.
 * @param brush Pointer to the background brush.
 */
void set_dc_bg_brush(PDC dc, PBRUSH brush);
/**
 * Set the fill color of the device context.
 * @param dc Pointer to the device context.
 * @param color Fill color.
 */
void set_dc_fill_color(PDC dc, RGBA color);
/**
 * Set the paint color of the device context.
 * @param dc Pointer to the device context.
 * @param color Paint color.
 */
void set_dc_paint_color(PDC dc, RGBA color);
/**
 * Set the text color of the device context.
 * @param dc Pointer to the device context.
 * @param color Paint color.
 */
void set_dc_txt_color(PDC dc, RGBA color);
/**
 * Set the background color of the device context.
 * @param dc Pointer to the device context.
 * @param color Background color.
 */
void set_dc_bg_color(PDC dc, RGBA color);

/*
 * DIBitmap
 */

/**
 * Create a device-independent bitmap.
 * @param width Pixel width of the DIB.
 * @param height Pixel height of the DIB.
 * @param depth Color depth of the DIB.
 * @return Pointer to the created device-independent bitmap.
 */
PDIB create_dib(u32 width, u32 height, u32 depth);
/**
 * Delete the specified device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap to be deleted.
 */
void delete_dib(PDIB dib);
/**
 * Load a .bmp file from disk into a device-independent bitmap.
 * @param filename File name of the .bmp file to be loaded.
 * @return Pointer to the created device-independent bitmap.
 */
PDIB load_dib(const char *filename);
/**
 * Save a device-independent bitmap into disk as a .bmp file.
 * @param dib Pointer to the device-independent bitmap to be saved.
 * @param filename File name of the .bmp file to be created.
 */
void save_dib(const PDIB dib, const char *filename);
/**
 * Get the pixel width of the device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @return Pixel width of the device-independent bitmap.
 */
u32 dib_get_width(const PDIB dib);
/**
 * Get the pixel height of the device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @return Pixel height of the device-independent bitmap.
 */
u32 dib_get_height(const PDIB dib);
/**
 * Get the color depth of the device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @return Color depth of the device-independent bitmap.
 */
u16 dib_get_depth(const PDIB dib);
/**
 * Get the r, g, b value in pixel (x, y) of the device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @param x The x-coordinate, in pixels.
 * @param y The y-coordinate, in pixels.
 * @param r Receiver pointer of R value.
 * @param g Receiver pointer of G value.
 * @param b Receiver pointer of B value.
 */
void dib_get_pixel_rgb(const PDIB dib, u32 x, u32 y, u8 *r, u8 *g, u8 *b);
/**
 * Set the r, g, b value in pixel (x, y) of the device-independent bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @param x The x-coordinate, in pixels, of the pixel.
 * @param y The y-coordinate, in pixels, of the pixel.
 * @param r The R value.
 * @param g The G value.
 * @param b The B value.
 */
void dib_set_pixel_rgb(PDIB dib, u32 x, u32 y, u8 r, u8 g, u8 b);
/**
 * Get the color index in palette of the pixel (x, y) of the device-independent
 * bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @param x The x-coordinate, in pixels, of the pixel.
 * @param y The y-coordinate, in pixels, of the pixel.
 * @param val Receiver pointer of the color index in palette.
 */
void dib_get_pixel_index(const PDIB dib, u32 x, u32 y, u8 *val);
/**
 * Set the color index in palette of the pixel (x, y) of the device-independent
 * bitmap.
 * @param dib Pointer to the device-independent bitmap.
 * @param x The x-coordinate, in pixels, of the pixel.
 * @param y The y-coordinate, in pixels, of the pixel.
 * @param val Color index in palette.
 */
void dib_set_pixel_index(PDIB dib, u32 x, u32 y, u8 val);
/**
 * Get the r, g, b value in palette of the device-independent bitmap by index.
 * @param dib Pointer to the device-independent bitmap.
 * @param index Color index in palette.
 * @param r Receiver pointer of R value.
 * @param g Receiver pointer of G value.
 * @param b Receiver pointer of B value.
 */
void dib_get_palette_color(const PDIB dib, u8 index, u8 *r, u8 *g, u8 *b);
/**
 * Set the r, g, b value in palette of the device-independent bitmap by index.
 * @param dib Pointer to the device-independent bitmap.
 * @param index Color index in palette.
 * @param r The R value.
 * @param g The G value.
 * @param b The B value.
 */
void dib_set_palette_color(PDIB dib, u8 index, u8 r, u8 g, u8 b);

/*
 * Font
 */

/**
 * Create a bit-font from fontbits_src.
 * @param fontbits_src Bits source of bit-font.
 * @param byte_width Width in bytes of single character.
 * @param byte_height Height in bytes of single character.
 * @param byte_reverse Reverse bytes. Used for little-endian.
 * @return Pointer to the created font.
 */
PFONT create_font(const char *fontbits_src, int byte_width, int byte_height,
                  int byte_reverse);
/**
 * Delete the specified font.
 * @param font Pointer to the font to be deleted.
 */
void delete_font(PFONT font);
/**
 * Draw a character.
 * @param dc Pointer to the device context where drawing takes place.
 * @param ch The character to be drawn.
 * @param x The x-coordinate, in pixels.
 * @param y The y-coordinate, in pixels.
 * @return 0 if succ.
 */
int draw_char(const PDC dc, char ch, int x, int y);

/*
 * Line
 */

/**
 * Draw a horizontal line.
 * @param dc Pointer to the device context where drawing takes place.
 * @param left The x-coordinate, in pixels, of the left bounder of the line.
 * @param right The x-coordinate, in pixels, of the right bounder of the line.
 * @param y The y-coordinate, in pixels, of the line.
 * @return 0 if succ.
 */
int draw_hline(const PDC dc, int left, int right, int y);
/**
 * Draw a vertical line.
 * @param dc Pointer to the device context where drawing takes place.
 * @param x The x-coordinate, in pixels, of the line.
 * @param top The y-coordinate, in pixels, of the top bounder of the line.
 * @param bottom The y-coordinate, in pixels, of the bottom bounder of the line.
 * @return 0 if succ.
 */
int draw_vline(const PDC dc, int x, int top, int bottom);
/**
 * Draw a line.
 * @param dc Pointer to the device context where drawing takes place.
 * @param x1 The x-coordinate, in pixels, of the line's starting point.
 * @param y1 The y-coordinate, in pixels, of the line's starting point.
 * @param x2 The x-coordinate, in pixels, of the line's ending point.
 * @param y2 The y-coordinate, in pixels, of the line's ending point.
 * @return 0 if succ.
 */
int draw_line(const PDC dc, int x1, int y1, int x2, int y2);
/**
 * Draw a rectangle.
 * @param dc Pointer to the device context where drawing takes place.
 * @param left The x-coordinate, in pixels, of the left bounder of the rect.
 * @param top The y-coordinate, in pixels, of the top bounder of the rect.
 * @param right The x-coordinate, in pixels, of the right bounder of the rect.
 * @param bottom The y-coordinate, in pixels, of the bottom bounder of the rect.
 * @return 0 if succ.
 */
int draw_rect(const PDC dc, int left, int top, int right, int bottom);
/**
 * Fill a rectangle.
 * @param dc Pointer to the device context where drawing takes place.
 * @param left The x-coordinate, in pixels, of the left bounder of the rect.
 * @param top The y-coordinate, in pixels, of the top bounder of the rect.
 * @param right The x-coordinate, in pixels, of the right bounder of the rect.
 * @param bottom The y-coordinate, in pixels, of the bottom bounder of the rect.
 * @return 0 if succ.
 */
int fill_rect(const PDC dc, int left, int top, int right, int bottom);

/*
 * Pixel
 */

/**
 * Draw a pixel.
 * @param dc Pointer to the device context where drawing takes place.
 * @param x The x-coordinate, in pixels.
 * @param y The y-coordinate, in pixels
 * @return 0 if succ.
 */
int draw_pixel(const PDC dc, int x, int y);
/**
 * Draw pixels.
 * @param dc Pointer to the device context where drawing takes place.
 * @param points A pointer to an array of points to be drawn.
 * @param nr_points Number of points in the array.
 * @return 0 if succ.
 */
int draw_pixels(const PDC dc, const POINT *points, int nr_points);

/*
 * Polygon
 */

/**
 * Draw a polygon.
 * @param dc Pointer to the device context where drawing takes place.
 * @param points A pointer to an array of vertices of the polygon.
 * @param nr_points Number of points in the array. (must >= 2)
 * @return 0 if succ.
 */
int draw_polygon(const PDC dc, const POINT *points, int nr_points);
/**
 * Draw a polyline.
 * @param dc Pointer to the device context where drawing takes place.
 * @param points A pointer to an array of vertices of the polyline.
 * @param nr_points Number of points in the array. (must >= 2)
 * @return 0 if succ.
 */
int draw_polyline(const PDC dc, const POINT *points, int nr_points);

/*
 * Region
 */

/**
 * Create an empty region.
 * @param size Initial size of the region.
 * @return Pointer to the created region.
 */
PRGN create_region(int size);
/**
 * Set a rectangle region from a rectangle.
 * @param left The x-coordinate, in pixels, of the left bounder of the rect.
 * @param top The y-coordinate, in pixels, of the top bounder of the rect.
 * @param right The x-coordinate, in pixels, of the right bounder of the rect.
 * @param bottom The y-coordinate, in pixels, of the bottom bounder of the rect.
 * @return 0 if succ.
 */
int set_rect_region(PRGN rgn, int left, int top, int right, int bottom);
/**
 * Set the rectangle region from a rectangle.
 * @param rc A pointer to the rectangle.
 * @return 0 if succ.
 */
int set_rc_region(PRGN rgn, const RECT *rc);
/**
 * Set the rectangle region from a device context.
 * @param dc Pointer to the device context.
 * @return 0 if succ.
 */
int set_dc_region(PRGN rgn, const PDC dc);
/**
 * Delete the specified region.
 * @param rgn Pointer to the region to be deleted.
 */
void delete_region(PRGN rgn);
/**
 * Get a rectangle of the region.
 * @param rgn Pointer to the region.
 * @param cmd Command of getting which rectangle.
 * @return A pointer to the rectangle.
 */
RECT *get_region_rect(PRGN rgn, enum GET_RGN_RC_CMD cmd);
/**
 * Combine src1 region and src2 region to dst region.
 * @param dst Pointer to destination region.
 * @param src1 Pointer to source1 region.
 * @param hsrc2 Pointer to source2 region.
 * @param mode Region combination mode.
 * @return 0 if succ.
 */
int combine_region(PRGN dst, PRGN src1, PRGN src2, enum RGN_COMB_MODE mode);
/**
 * Clear the specified region to an empty region.
 * @param rgn Pointer to the region to be cleared.
 */
void clear_region(PRGN rgn);
/**
 * Move the specified region by adding (x, y).
 * @param rgn Pointer to the region.
 * @param x The offset in x-coordinate, in pixels.
 * @param y The offset in x-coordinate, in pixels.
 */
void move_region(PRGN rgn, int x, int y);
/**
 * Check if point (x, y) locates in the region.
 * @param rgn Pointer to the region.
 * @param x The x-coordinate, in pixels, of the point.
 * @param y The y-coordinate, in pixels, of the point.
 * @return True if (x, y) locates in the region. Otherwise false.
 */
bool pt_in_region(const PRGN rgn, int x, int y);
/**
 * Check if the specified region is empty.
 * @param rgn Pointer to the region.
 * @return True if the region is empty. Otherwise false.
 */
bool region_null(const PRGN rgn);
/**
 * Print information of the specified region.
 * @param rgn Pointer to the region.
 */
void print_region(const PRGN rgn);
