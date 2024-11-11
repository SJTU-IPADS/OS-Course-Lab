#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <lvgl.h>
#include "lv_drivers/wayland/wayland.h"

#define H_RES (900)
#define V_RES (900)

#define LOG(...) printf(__VA_ARGS__)

typedef lv_style_t Style;

extern Style basic_style;
extern Style style_I; 
extern Style style_I_preview;
extern Style style_J;
extern Style style_J_preview;
extern Style style_L;
extern Style style_L_preview;
extern Style style_O;
extern Style style_O_preview;
extern Style style_S;
extern Style style_S_preview;
extern Style style_Z;
extern Style style_Z_preview;
extern Style style_T;
extern Style style_T_preview;
extern Style not_placed_style;

typedef struct {
    int repaint;
    Style *old_style;
    Style *new_style;
    //Style *style;
    lv_obj_t *lv_obj;
} DisplayObject;

inline void set_style(DisplayObject *obj, Style *style) {
    obj->new_style = style;
    //obj->style = style;
}

inline Style *get_style(DisplayObject *obj) {
    return obj->new_style;
    //return obj->style;
}

inline void instantly_repaint_object(DisplayObject *obj) {
    //if (obj && obj->lv_obj && obj->style) {
    if (obj && obj->lv_obj && obj->new_style) {
        //lv_obj_remove_style_all(obj->lv_obj);
        //lv_obj_add_style(obj->lv_obj, &basic_style, 0);
        //lv_obj_add_style(obj->lv_obj, obj->style, 0);
        
        if (obj->old_style) {
            lv_obj_remove_style(obj->lv_obj, obj->old_style, 0);
        }
        lv_obj_add_style(obj->lv_obj, obj->new_style, 0);
        obj->old_style = obj->new_style;
    }
}

inline void mark_repaint(DisplayObject *obj) {
    instantly_repaint_object(obj);
    obj->repaint = 3;
}

inline void clear_repaint(DisplayObject *obj) {
    obj->repaint = 0;
}

void do_repaint();
void show_score(int score, int level);
void show_pause_menu();
void show_main_menu();
void show_game_screen();
void show_settings();
void show_game_over();

extern int alive;

#endif