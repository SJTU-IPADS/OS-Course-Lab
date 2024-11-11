#ifndef __STYLE_H__
#define __STYLE_H__
#include "lv_drivers/wayland/wayland.h"

const lv_style_const_prop_t basic_style_props[] = {
   LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
   LV_STYLE_CONST_BORDER_WIDTH(2),
   LV_STYLE_CONST_ARC_ROUNDED(0)
};

const lv_style_const_prop_t style_I_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 220, 220)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 255, 255)),
};

const lv_style_const_prop_t style_I_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 120, 120)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 155, 155)),
};

const lv_style_const_prop_t style_J_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 0, 220)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 0, 255)),
};

const lv_style_const_prop_t style_J_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 0, 150)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 0, 155)),
};

const lv_style_const_prop_t style_L_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(220, 110, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(255, 122, 0)),
};

const lv_style_const_prop_t style_L_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(150, 125, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(155, 130, 0)),
};

const lv_style_const_prop_t style_O_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(110, 220, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(122, 255, 0)),
};

const lv_style_const_prop_t style_O_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(125, 150, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(130, 155, 0)),
};

const lv_style_const_prop_t style_S_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 220, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 255, 0)),
};

const lv_style_const_prop_t style_S_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(0, 150, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(0, 155, 0)),
};

const lv_style_const_prop_t style_Z_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(255, 0, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(255, 0, 0)),
};

const lv_style_const_prop_t style_Z_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(150, 0, 0)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(155, 0, 0)),
};

const lv_style_const_prop_t style_T_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(220, 0, 220)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(225, 0, 255)),
};

const lv_style_const_prop_t style_T_preview_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(150, 0, 150)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(155, 0, 155)),
};

const lv_style_const_prop_t not_placed_style_props[] = {
   LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE32(20, 20, 20)),
   LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE32(50, 50, 50)),
};

#define INIT_STYLE(name)       LV_STYLE_CONST_INIT(name, name##_props)

INIT_STYLE(basic_style);
INIT_STYLE(style_I); 
INIT_STYLE(style_I_preview);
INIT_STYLE(style_J);
INIT_STYLE(style_J_preview);
INIT_STYLE(style_L);
INIT_STYLE(style_L_preview);
INIT_STYLE(style_O);
INIT_STYLE(style_O_preview);
INIT_STYLE(style_S);
INIT_STYLE(style_S_preview);
INIT_STYLE(style_Z);
INIT_STYLE(style_Z_preview);
INIT_STYLE(style_T);
INIT_STYLE(style_T_preview);
INIT_STYLE(not_placed_style);
#endif