#include "display.h"
#include "tetris.h"

int alive = 1;

/* INPUT */

static struct {
    void (*original_cb)(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
    uint32_t key;
    lv_indev_state_t state;
} __keyboard_state;

static void keyboard_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    __keyboard_state.original_cb(drv, data);
    if (data->key != __keyboard_state.key || data->state != __keyboard_state.state) {
        __keyboard_state.key = data->key;
        __keyboard_state.state = data->state;
        keyboard_dispatch(data->key, data->state);
    }
}

/* DISPLAY */

#define GRID_SIDE_LEN 40
#define PREVIEW_GRID_SIDE_LEN 20

static lv_coord_t col_dsc[FIELD_WIDTH + 1];
static lv_coord_t row_dsc[GAME_OVER_HEIGHT + 1];

static lv_coord_t np_col_dsc[NEXT_PREVIEW_WIDTH + 1];
static lv_coord_t np_row_dsc[NEXT_PREVIEW_HEIGHT + 1];

static lv_coord_t np_col_dsc_small_1[NEXT_PREVIEW_WIDTH + 1];
static lv_coord_t np_row_dsc_small_1[NEXT_PREVIEW_HEIGHT + 1];

static lv_coord_t np_col_dsc_small_2[NEXT_PREVIEW_WIDTH + 1];
static lv_coord_t np_row_dsc_small_2[NEXT_PREVIEW_HEIGHT + 1];

static lv_obj_t *field_score_label;
static lv_obj_t *game_over_menu_score_label;

static void lv_register_input_handlers() {
    lv_indev_t *keyboard = lv_wayland_get_keyboard(lv_disp_get_default());
    lv_indev_drv_t *keyboard_drv = keyboard->driver;
    __keyboard_state.original_cb = keyboard_drv->read_cb;
    keyboard_drv->read_cb = keyboard_read_cb;
}

static void create_grid_matrix_from_dsc(lv_obj_t *grid_matrix_object, lv_coord_t col_dsc[], lv_coord_t row_dsc[], int width, int height, int side_len) {
    for (int x = 0; x < width; ++x) {
        col_dsc[x] = side_len;
    }
    for (int y = 0; y < height; ++y) {
        row_dsc[y] = side_len;
    }
    col_dsc[width] = LV_GRID_TEMPLATE_LAST;
    row_dsc[height] = LV_GRID_TEMPLATE_LAST;

    lv_obj_remove_style_all(grid_matrix_object);
    lv_obj_set_style_pad_column(grid_matrix_object, 0, 0);
    lv_obj_set_style_grid_column_dsc_array(grid_matrix_object, col_dsc, 0);
    lv_obj_set_style_pad_row(grid_matrix_object, 0, 0);
    lv_obj_set_style_grid_row_dsc_array(grid_matrix_object, row_dsc, 0);
    lv_obj_set_size(grid_matrix_object, side_len *width, side_len *height);
    lv_obj_set_layout(grid_matrix_object, LV_LAYOUT_GRID);
}

static lv_obj_t *init_field_screen() {
    /*Create a container with grid*/
    LOG("lv_field -> lv_obj_create\n");
    //lv_obj_t *window = lv_scr_act();
    lv_obj_t *field_screen = lv_obj_create(NULL);
    lv_obj_t *field_display = lv_obj_create(field_screen);
    create_grid_matrix_from_dsc(field_display, col_dsc, row_dsc, FIELD_WIDTH, GAME_OVER_HEIGHT, GRID_SIDE_LEN);
    lv_obj_align(field_display, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *next_display[3] = { lv_obj_create(field_screen), lv_obj_create(field_screen), lv_obj_create(field_screen) };
    create_grid_matrix_from_dsc(next_display[0], np_col_dsc, np_row_dsc, NEXT_PREVIEW_WIDTH, NEXT_PREVIEW_HEIGHT, GRID_SIDE_LEN);
    lv_obj_align_to(next_display[0], field_display, LV_ALIGN_OUT_RIGHT_TOP, 60, 0);
    create_grid_matrix_from_dsc(next_display[1], np_col_dsc_small_1, np_row_dsc_small_1, NEXT_PREVIEW_WIDTH, NEXT_PREVIEW_HEIGHT, PREVIEW_GRID_SIDE_LEN);
    lv_obj_align_to(next_display[1], next_display[0], LV_ALIGN_OUT_BOTTOM_LEFT, -10, 20);
    create_grid_matrix_from_dsc(next_display[2], np_col_dsc_small_2, np_row_dsc_small_2, NEXT_PREVIEW_WIDTH, NEXT_PREVIEW_HEIGHT, PREVIEW_GRID_SIDE_LEN);
    lv_obj_align_to(next_display[2], next_display[0], LV_ALIGN_OUT_BOTTOM_LEFT, 90, 20);

    field_score_label = lv_label_create(field_screen);
    lv_obj_set_width(field_score_label, 100);
    lv_obj_set_style_text_align(field_score_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(field_score_label, next_display[0], LV_ALIGN_OUT_BOTTOM_MID, 0, 10 + NEXT_PREVIEW_HEIGHT *PREVIEW_GRID_SIDE_LEN + 20);

    LOG("lv_field -> grid\n");
    lv_obj_t *obj;

    int col, row;
    for (int i = 0; i < FIELD_WIDTH *FIELD_HEIGHT; ++i) {
        col = i % FIELD_WIDTH;
        row = i / FIELD_WIDTH;

        if (row < GAME_OVER_HEIGHT) {
            obj = lv_obj_create(field_display);
            lv_obj_remove_style_all(obj);
            lv_obj_add_style(obj, &basic_style, 0);
            lv_obj_add_style(obj, &not_placed_style, 0);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
                                    LV_GRID_ALIGN_STRETCH, GAME_OVER_HEIGHT - 1 - row, 1);
            
            grid_at(field, col, row).display_object.lv_obj = obj;
            grid_at(field, col, row).display_object.old_style = &not_placed_style;
        } else {
            grid_at(field, col, row).display_object.lv_obj = NULL;
        }
    }

    for (int j = 0; j < 3; ++j) {
        for (int i = 0; i < NEXT_PREVIEW_WIDTH *NEXT_PREVIEW_HEIGHT; ++i) {
            col = i % NEXT_PREVIEW_WIDTH;
            row = i / NEXT_PREVIEW_WIDTH;

            obj = lv_obj_create(next_display[j]);
            lv_obj_remove_style_all(obj);
            lv_obj_add_style(obj, &basic_style, 0);
            lv_obj_add_style(obj, &not_placed_style, 0);
            lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
                                    LV_GRID_ALIGN_STRETCH, NEXT_PREVIEW_HEIGHT - 1 - row, 1);

            grid_at(next_preview[j], col, row).display_object.lv_obj = obj;
            grid_at(next_preview[j], col, row).display_object.old_style = &not_placed_style;
        }
    }

    lv_register_input_handlers();
    return field_screen;
}

static void btn_start_game_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_clean(lv_layer_top());
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        show_game_screen();
        start_game();
    }
}

static void btn_info_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        show_info_menu();
    }
}

static void btn_quit_game_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        alive = 0;
    }
}

static void btn_resume_game_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        /* Close menu */
        lv_obj_clean(lv_layer_top());
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        resume_game();
    }
}

static void btn_return_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        /* Close menu */
        lv_obj_clean(lv_layer_top());
        lv_obj_clear_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
        show_main_menu();
    }
}

static lv_obj_t *init_main_menu_screen() {
    lv_obj_t *main_menu_screen = lv_obj_create(NULL);

    lv_obj_t *label;

    lv_obj_t *main_menu_label = lv_label_create(main_menu_screen);
    lv_obj_set_width(main_menu_label, 160);
    lv_obj_set_style_text_align(main_menu_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(main_menu_label, LV_ALIGN_CENTER, 0, -140);
    lv_label_set_text(main_menu_label, "TETRIS");

    lv_obj_t *btn_start_game = lv_btn_create(main_menu_screen);
    lv_obj_set_width(btn_start_game, 160);
    lv_obj_add_event_cb(btn_start_game, btn_start_game_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_start_game, LV_ALIGN_CENTER, 0, -60);

    label = lv_label_create(btn_start_game);
    lv_label_set_text(label, "NEW GAME");
    lv_obj_center(label);

    lv_obj_t *btn_info = lv_btn_create(main_menu_screen);
    lv_obj_set_width(btn_info, 160);
    lv_obj_add_event_cb(btn_info, btn_info_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_info, LV_ALIGN_CENTER, 0, 0);

    label = lv_label_create(btn_info);
    lv_label_set_text(label, "INFO");
    lv_obj_center(label);

    lv_obj_t *btn_quit_game = lv_btn_create(main_menu_screen);
    lv_obj_set_width(btn_quit_game, 160);
    lv_obj_add_event_cb(btn_quit_game, btn_quit_game_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_quit_game, LV_ALIGN_CENTER, 0, 60);

    label = lv_label_create(btn_quit_game);
    lv_label_set_text(label, "QUIT");
    lv_obj_center(label);

    return main_menu_screen;
}

static lv_obj_t *init_pause_menu() {
    lv_obj_t *pause_menu = lv_obj_create(lv_layer_top());

    lv_obj_set_size(pause_menu, 240, 300);
    lv_obj_center(pause_menu);

    lv_obj_t *label;

    lv_obj_t *pause_menu_label = lv_label_create(pause_menu);
    lv_obj_set_width(pause_menu_label, 160);
    lv_obj_set_style_text_align(pause_menu_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(pause_menu_label, LV_ALIGN_CENTER, 0, -100);
    lv_label_set_text(pause_menu_label, "PAUSED");

    lv_obj_t *btn_resume_game = lv_btn_create(pause_menu);
    lv_obj_set_width(btn_resume_game, 160);
    lv_obj_add_event_cb(btn_resume_game, btn_resume_game_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_resume_game, LV_ALIGN_CENTER, 0, -20);

    label = lv_label_create(btn_resume_game);
    lv_label_set_text(label, "RESUME");
    lv_obj_center(label);

    lv_obj_t *btn_start_game = lv_btn_create(pause_menu);
    lv_obj_set_width(btn_start_game, 160);
    lv_obj_add_event_cb(btn_start_game, btn_start_game_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_start_game, LV_ALIGN_CENTER, 0, 40);

    label = lv_label_create(btn_start_game);
    lv_label_set_text(label, "NEW GAME");
    lv_obj_center(label);

    lv_obj_t *btn_return = lv_btn_create(pause_menu);
    lv_obj_set_width(btn_return, 160);
    lv_obj_add_event_cb(btn_return, btn_return_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_return, LV_ALIGN_CENTER, 0, 100);

    label = lv_label_create(btn_return);
    lv_label_set_text(label, "MAIN MENU");
    lv_obj_center(label);

    return pause_menu;
}

static lv_obj_t *init_game_over_menu() {
    lv_obj_t *game_over_menu = lv_obj_create(lv_layer_top());

    lv_obj_set_size(game_over_menu, 240, 300);
    lv_obj_center(game_over_menu);

    lv_obj_t *game_over_menu_title = lv_label_create(game_over_menu);
    lv_obj_set_width(game_over_menu_title, 160);
    lv_obj_set_style_text_align(game_over_menu_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(game_over_menu_title, LV_ALIGN_CENTER, 0, -100);
    lv_label_set_text(game_over_menu_title, "GAME OVER!");

    game_over_menu_score_label = lv_label_create(game_over_menu);
    lv_obj_set_width(game_over_menu_score_label, 160);
    lv_obj_set_style_text_align(game_over_menu_score_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(game_over_menu_score_label, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label;

    lv_obj_t *btn_start_game = lv_btn_create(game_over_menu);
    lv_obj_set_width(btn_start_game, 160);
    lv_obj_add_event_cb(btn_start_game, btn_start_game_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_start_game, LV_ALIGN_CENTER, 0, 40);

    label = lv_label_create(btn_start_game);
    lv_label_set_text(label, "NEW GAME");
    lv_obj_center(label);

    lv_obj_t *btn_return = lv_btn_create(game_over_menu);
    lv_obj_set_width(btn_return, 160);
    lv_obj_add_event_cb(btn_return, btn_return_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_return, LV_ALIGN_CENTER, 0, 100);

    label = lv_label_create(btn_return);
    lv_label_set_text(label, "MAIN MENU");
    lv_obj_center(label);

    return game_over_menu;
}

static void init_settings_screen() {
    
}

static lv_obj_t *init_info_menu() {
    lv_obj_t *info_menu = lv_obj_create(lv_layer_top());

    lv_obj_set_size(info_menu, 300, 360);
    lv_obj_center(info_menu);

    lv_obj_t *info_menu_title = lv_label_create(info_menu);
    lv_obj_set_width(info_menu_title, 160);
    lv_obj_set_style_text_align(info_menu_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(info_menu_title, LV_ALIGN_CENTER, 0, -100);
    lv_label_set_text(info_menu_title, "ABOUT");

    lv_obj_t *info_label = lv_label_create(info_menu);
    lv_obj_set_width(info_label, 270);
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 20);
    lv_label_set_text(info_label, "Tetris game for ChCore\nDependencies:ChCore, Wayland, LVGL\nCredits:The original Tetris game\nAuthor:ART1st");

    lv_obj_t *btn_return = lv_btn_create(info_menu);
    lv_obj_add_event_cb(btn_return, btn_return_event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn_return, LV_ALIGN_CENTER, 0, 140);

    lv_obj_t *label = lv_label_create(btn_return);
    lv_label_set_text(label, "RETURN");
    lv_obj_center(label);

    return info_menu;
}

/* INTERFACE */

void do_repaint() {
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < GAME_OVER_HEIGHT; ++y) {
            DisplayObject *obj = &grid_at(field, x, y).display_object;
            if (obj->repaint) {
                instantly_repaint_object(obj);
                obj->repaint -= 1;
            }
        }
    }
}

void show_score(int score, int level) {
    lv_label_set_text_fmt(field_score_label, "Score: %d\n Level: %d", score, level);
}

void show_special_effect() {

}

void show_pause_menu() {
    lv_obj_clean(lv_layer_top());
    init_pause_menu();
    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_set_parent(pause_menu, lv_layer_top());
}

void show_info_menu() {
    lv_obj_clean(lv_layer_top());
    init_info_menu();
    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_set_parent(info_menu, lv_layer_top());
}

/* TODO: why it stops? */
void show_main_menu() {
    lv_obj_clean(lv_scr_act());
    lv_scr_load(init_main_menu_screen());
}

void show_game_screen() {
    lv_obj_clean(lv_scr_act());
    lv_scr_load(init_field_screen());
}

void show_settings() {
}

void show_game_over(int score) {
    lv_obj_clean(lv_layer_top());
    init_game_over_menu();
    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    //lv_obj_set_parent(game_over_menu, lv_layer_top());
    lv_label_set_text_fmt(game_over_menu_score_label, "Your final score is: %d\n", score);
}
