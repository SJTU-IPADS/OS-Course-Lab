#include "tetriminos.h"
#include "display.h"

// TODO
Position wall_kick_table_i[8][NUM_WALL_KICK_TESTS] = {
    {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}},
    {{0, 0}, {1,  0}, { 1, -1}, {0,  2}, { 1,  2}},
    {{0, 0}, {1,  0}, { 1, -1}, {0,  2}, { 1,  2}},
    {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}},
    {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}},
    {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}},
};

Position wall_kick_table[8][NUM_WALL_KICK_TESTS] = {
    {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}},
    {{0, 0}, {1,  0}, { 1, -1}, {0,  2}, { 1,  2}},
    {{0, 0}, {1,  0}, { 1, -1}, {0,  2}, { 1,  2}},
    {{0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2}},
    {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}},
    {{0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2}},
    {{0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2}},
};

const int I[4][4][4] = {
    {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    },{
        {0, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 0, 0},
        {0, 1, 0, 0},
    },{
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
    },{
        {0, 0, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 0},
        {0, 0, 1, 0},
    }
};
const TetriminoTemplate template_I = {
    4, 4,
    I,
    wall_kick_table_i,
    &style_I, &style_I_preview,
    {-2, -2}
};

const int J[4][3][3] = {
    {
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 0},
    },{
        {0, 1, 1},
        {0, 1, 0},
        {0, 1, 0},
    },{
        {0, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
    },{
        {0, 1, 0},
        {0, 1, 0},
        {1, 1, 0},
    }
};
const TetriminoTemplate template_J = {
    3, 3,
    J,
    wall_kick_table,
    &style_J, &style_J_preview,
    {-2, -2}
};

const int L[4][3][3] = {
    {
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 0},
    },{
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 1},
    },{
        {0, 0, 0},
        {1, 1, 1},
        {1, 0, 0},
    },{
        {1, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
    }
};
const TetriminoTemplate template_L = {
    3, 3,
    L,
    wall_kick_table,
    &style_L, &style_L_preview,
    {-2, -2}
};

const int O[4][2][2] = {
    {
        {1, 1},
        {1, 1},
    },{
        {1, 1},
        {1, 1},
    },{
        {1, 1},
        {1, 1},
    },{
        {1, 1},
        {1, 1},
    }
};
const TetriminoTemplate template_O = {
    2, 2,
    O,
    wall_kick_table,
    &style_O, &style_O_preview,
    {-1, -1}
};

const int S[4][3][3] = {
    {
        {0, 1, 1},
        {1, 1, 0},
        {0, 0, 0},
    },{
        {0, 1, 0},
        {0, 1, 1},
        {0, 0, 1},
    },{
        {0, 0, 0},
        {0, 1, 1},
        {1, 1, 0},
    },{
        {1, 0, 0},
        {1, 1, 0},
        {0, 1, 0},
    }
};
const TetriminoTemplate template_S = {
    3, 3,
    S,
    wall_kick_table,
    &style_S, &style_S_preview,
    {-2, -2}
};

const int Z[4][3][3] = {
    {
        {1, 1, 0},
        {0, 1, 1},
        {0, 0, 0},
    },{
        {0, 0, 1},
        {0, 1, 1},
        {0, 1, 0},
    },{
        {0, 0, 0},
        {1, 1, 0},
        {0, 1, 1},
    },{
        {0, 1, 0},
        {1, 1, 0},
        {1, 0, 0},
    }
};
const TetriminoTemplate template_Z = {
    3, 3,
    Z,
    wall_kick_table,
    &style_Z, &style_Z_preview,
    {-2, -2}
};

const int T[4][3][3] = {
    {
        {0, 1, 0},
        {1, 1, 1},
        {0, 0, 0},
    },{
        {0, 1, 0},
        {0, 1, 1},
        {0, 1, 0},
    },{
        {0, 0, 0},
        {1, 1, 1},
        {0, 1, 0},
    },{
        {0, 1, 0},
        {1, 1, 0},
        {0, 1, 0},
    }
};
const TetriminoTemplate template_T = {
    3, 3,
    T,
    wall_kick_table,
    &style_T, &style_T_preview,
    {-2, -2}
};

const TetriminoTemplate *templates[NUM_TEMPLATES] = {
    &template_I,
    &template_J,
    &template_L,
    &template_O,
    &template_S,
    &template_Z,
    &template_T,
};