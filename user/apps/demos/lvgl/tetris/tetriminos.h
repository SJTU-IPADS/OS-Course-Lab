#ifndef __TETRIMINOS_H__
#define __TETRIMINOS_H__

#include "display.h"

#define NUM_TEMPLATES 7
#define NUM_WALL_KICK_TESTS 5

typedef struct {
    int X;
    int Y;
} Position;

typedef struct {
    const int width;
    const int height;
    const int *blocks;
    const Position (*wall_kick_table)[8][NUM_WALL_KICK_TESTS];
    const Style *style;
    const Style *style_preview;
    const Position spawn_position;
} TetriminoTemplate;

typedef struct {
    const TetriminoTemplate *type;
    int direction;
    Position position;
    int visible;
    const Style *style;
} Tetrimino;

#define tetrimino_relative_block(t, d, x, y) (*(t->type->blocks + d * t->type->width * t->type->height + (t->type->height - 1 - y) * t->type->width + x))

extern const TetriminoTemplate template_I;
extern const TetriminoTemplate template_J;
extern const TetriminoTemplate template_L;
extern const TetriminoTemplate template_O;
extern const TetriminoTemplate template_S;
extern const TetriminoTemplate template_Z;
extern const TetriminoTemplate template_T;
extern const TetriminoTemplate *templates[];

#endif