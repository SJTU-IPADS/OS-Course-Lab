#ifndef __TETRIS_H__
#define __TETRIS_H__

#define MOVE_LEFT                   1
#define MOVE_RIGHT                  2
#define MOVE_DOWN                   3
#define MOVE_GRAVITY                4
#define MOVE_HARD_DROP              5
#define ROTATE_CLOCKWISE            11
#define ROTATE_COUNTER_CLOCKWISE    12

#define LOCKDOWN_CLASSIC  1
#define LOCKDOWN_MOVEMENT 2
#define LOCKDOWN_INFINITE 3

#define MSPT 2
#define REFRESH_INTERVAL_MS 10

#include "display.h"
#include "tetriminos.h"

typedef struct {
    int filled;
    DisplayObject display_object;
} FieldGrid;

#define for_each_grid(t, x, y) \
for (int x = 0; x < t->type->width; ++x) \
    for (int y = 0; y < t->type->height; ++y)

#define for_each_filled_grid(t, x, y) \
for (int x = 0; x < t->type->width; ++x) \
    for (int y = 0; y < t->type->height; ++y) \
        if (tetrimino_relative_block(t, t->direction, x, y))

#define FIELD_HEIGHT 24
#define FIELD_WIDTH 10

#define NEXT_PREVIEW_HEIGHT 4
#define NEXT_PREVIEW_WIDTH 4

#define GAME_OVER_HEIGHT 20

FieldGrid field[FIELD_HEIGHT][FIELD_WIDTH];
FieldGrid next_preview[3][NEXT_PREVIEW_HEIGHT][NEXT_PREVIEW_WIDTH];
#define grid_at(f, x, y) (f[y][x])

typedef uint32_t Key;
#define KEY_PRESSED     1
#define KEY_RELEASED    0

int tetrimino_tick(int tick);
void keyboard_dispatch(Key key, int state);
void init_field();
void start_game();
void resume_game();

#endif