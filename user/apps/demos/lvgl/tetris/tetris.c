#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "tetris.h"
#include "tetriminos.h"
#include "rand.h"
#include "display.h"
#include "list.h"

#define MAX_LEVEL 6

static struct Settings {
    int mspt;
    int enable_wall_kick;
    int das_delay;
    int ar_ticks;
    //int are;
    int lockdown_strategy;
    int max_movement_lockdown_reset_counter;
    int max_combo;
    int gravity[MAX_LEVEL];
    int lockdown_delay[MAX_LEVEL];
    int level_up_score[MAX_LEVEL - 1];
    int enable_dropdown_preview;
    struct {
        Key LEFT;
        Key RIGHT;
        Key DOWN;
        Key HARD_DROP;
        Key CLOCKWISE;
        Key COUNTER_CLOCKWISE;
        Key PAUSE;
    } key_map;
} settings = {
    .mspt = MSPT,
    .ar_ticks = 50 / MSPT,
    .das_delay = 300 / MSPT,
    .enable_wall_kick = 1,
    .lockdown_strategy = LOCKDOWN_MOVEMENT,
    .max_movement_lockdown_reset_counter = 15,
    .gravity = {
        750 / MSPT,
        600 / MSPT,
        500 / MSPT,
        420 / MSPT,
        350 / MSPT,
        300 / MSPT,
    },
    .lockdown_delay = {
        500 / MSPT,
        450 / MSPT,
        350 / MSPT,
        270 / MSPT,
        200 / MSPT,
        150 / MSPT,
    },
    .level_up_score = {
        1000,
        2000,
        3000,
        5000,
        8000,
    },
    .enable_dropdown_preview = 1,
    .key_map = {
        .LEFT = 0x04,
        .RIGHT = 0x07,
        .DOWN = 0x16,
        .CLOCKWISE = 0x08,
        .COUNTER_CLOCKWISE = 0x14,
        .PAUSE = 0x29,
    },
    .max_combo = 6
};

static struct {
    int paused;
    //Tetrimino *temp_storage;
    Tetrimino *next_tetriminos[3];
    Tetrimino *active_tetrimino;
    Tetrimino *dropdown_preview_tetrimino;
    int lockdown_delay;
    int gravity_ticks;
    int gravity_tick_counter;
    int lockdown_tick_counter;
    int movement_lockdown_reset_counter;
    int over;
    struct {
        int score;
        int level;
        int b2b;
        int combo;
    } score;
    struct {
        int op;
        int ar_tick_counter;
        int das_tick_counter;
    } longpress_state;
    ListHead keyboard_pressed_keys;
} game;

static inline int counter_countdown(int *counter) {
    if (*counter == 0) {
        return 1;
    }
    if (*counter > 0) {
        --(*counter);
    }
    return 0;
}

static inline void set_counter(int *counter, int value) {
    *counter = value;
}

static inline void disable_counter(int *counter) {
    *counter = -1;
}

static inline int grid_is_empty(Position *pos) {
    if (pos->X < 0 || pos->X >= FIELD_WIDTH
        || pos->Y < 0 || pos->Y >= FIELD_HEIGHT) {
        return 0;
    }
    return !grid_at(field, pos->X, pos->Y).filled;
}

static int test_placement(Tetrimino *t, Position *delta, int direction) {
    Position grid;
    for_each_grid(t, dx, dy) {
        if (tetrimino_relative_block(t, direction, dx, dy)) {
            grid.X = t->position.X + dx + delta->X;
            grid.Y = t->position.Y + dy + delta->Y;
            if (!grid_is_empty(&grid)) {
                return 0;
            }
        }
    }
    return 1;
}

static Position _down = {.X = 0, .Y = -1};
static int should_lock(Tetrimino *t) {
    return !test_placement(t, &_down, t->direction);
}

static void render_next_tetrimino_preview(Tetrimino *t, FieldGrid preview_field[NEXT_PREVIEW_HEIGHT][NEXT_PREVIEW_WIDTH]) {
    for (int x = 0; x < NEXT_PREVIEW_WIDTH; ++x) {
        for (int y = 0; y < NEXT_PREVIEW_HEIGHT; ++y) {
            DisplayObject *obj = &grid_at(preview_field, x, y).display_object;
            set_style(obj, &not_placed_style);
            mark_repaint(obj);
        }
    }
    for_each_filled_grid(t, dx, dy) {
        DisplayObject *obj = &grid_at(preview_field, NEXT_PREVIEW_WIDTH / 2 + t->type->spawn_position.X + dx, NEXT_PREVIEW_HEIGHT / 2 + t->type->spawn_position.Y + dy).display_object;
        set_style(obj, t->style);
        mark_repaint(obj);
    }
}

static void repaint_tetrimino_with_style(Tetrimino *t, Style *style) {
    for_each_filled_grid(t, dx, dy) {
        DisplayObject *obj = &grid_at(field, t->position.X + dx, t->position.Y + dy).display_object;
        set_style(obj, style);
        mark_repaint(obj);
    }
}

static inline void update_position(Tetrimino *t, Position *delta, int new_direction) {
    t->position.X += delta->X;
    t->position.Y += delta->Y;
    if (new_direction >= 0) {
        t->direction = new_direction;
    }
}

static int super_rotate(Tetrimino *t, int kick_op_index, int new_direction) {
    if (settings.enable_wall_kick) {
        for (int i = 0; i < NUM_WALL_KICK_TESTS; ++i) {
            //LOG("kick table i %d, new_direction %d\n", i, new_direction);
            if (test_placement(t, &((*t->type->wall_kick_table)[kick_op_index][i]), new_direction)) {
                update_position(t, &((*t->type->wall_kick_table)[kick_op_index][i]), new_direction);
                return 1;
            }
        }
    } else {
        if (test_placement(t, &((*t->type->wall_kick_table)[kick_op_index][0]), new_direction)) {
            update_position(t, &((*t->type->wall_kick_table)[kick_op_index][0]), new_direction);
            return 1;
        }
    }
    return 0;
}

static int do_tetrimino_op(Tetrimino *t, int op) {
    if (op == MOVE_LEFT || op == MOVE_RIGHT || op == MOVE_DOWN || op == MOVE_GRAVITY) {
        Position d = { 0, 0 };
        switch (op) {
            case MOVE_LEFT:
                d.X = -1;
                break;
            case MOVE_RIGHT:
                d.X = 1;
                break;
            case MOVE_DOWN:
            case MOVE_GRAVITY:
                d.Y = -1;
                break;
        }
        if (test_placement(t, &d, t->direction)) {
            update_position(t, &d, -1);
            return 1;
        }
    } else if (op == MOVE_HARD_DROP) {
        while (do_tetrimino_op(t, MOVE_DOWN)) {};
        return 0;
    } else {
        int wall_kick_op_index;
        int new_direction;
        if (op == ROTATE_CLOCKWISE) {
            wall_kick_op_index = t->direction * 2;
            new_direction = (t->direction + 1) % 4;
        } else {
            wall_kick_op_index = (t->direction * 2 - 1 + 8) % 8;
            new_direction = (t->direction - 1 + 4) % 4;
        }
        return super_rotate(t, wall_kick_op_index, new_direction);
    }
    return 0;
}

static void reset_game() {
    free(game.next_tetriminos[0]);
    free(game.next_tetriminos[1]);
    free(game.next_tetriminos[2]);
    if (game.active_tetrimino != NULL) free(game.active_tetrimino);
    free(game.dropdown_preview_tetrimino);
    empty_list(&game.keyboard_pressed_keys);
    game.over = 0;
    game.paused = 1;
}

static void game_over() {
    game.over = 1;
    LOG("GAME OVER!\n");
    show_game_over();
}

static inline void render_tetrimino_drop_preview() {
    if (settings.enable_dropdown_preview) {
        if (game.dropdown_preview_tetrimino->style) {
            repaint_tetrimino_with_style(game.dropdown_preview_tetrimino, &not_placed_style);
        }
        memcpy(game.dropdown_preview_tetrimino, game.active_tetrimino, sizeof(*game.dropdown_preview_tetrimino));
        do_tetrimino_op(game.dropdown_preview_tetrimino, MOVE_HARD_DROP);
        repaint_tetrimino_with_style(game.dropdown_preview_tetrimino, game.dropdown_preview_tetrimino->type->style_preview);
    }
}

int tetrimino_op(int op) {
    //LOG("op: %d\n", op);
    repaint_tetrimino_with_style(game.active_tetrimino, &not_placed_style);
    if (settings.lockdown_strategy == LOCKDOWN_MOVEMENT) {
        if (should_lock(game.active_tetrimino) && op != MOVE_GRAVITY) {
            if (counter_countdown(&game.movement_lockdown_reset_counter)) {
                return 0;
            }
        }
        if (op == MOVE_GRAVITY) {
            set_counter(&game.movement_lockdown_reset_counter, settings.max_movement_lockdown_reset_counter);
        }
    }
    if (do_tetrimino_op(game.active_tetrimino, op)) {
        if (should_lock(game.active_tetrimino)) {
            set_counter(&game.lockdown_tick_counter, game.lockdown_delay);
        } else {
            disable_counter(&game.lockdown_tick_counter);
            render_tetrimino_drop_preview();
        }
    }
    //LOG("tetrimino_op\n");
    repaint_tetrimino_with_style(game.active_tetrimino, game.active_tetrimino->style);
}

static inline int is_t_spin(Tetrimino *t) {
    if (t->type == &template_T) {
        int i = 0;
        i += grid_at(field, t->position.X, t->position.Y).filled;
        i += grid_at(field, t->position.X, t->position.Y + 2).filled;
        i += grid_at(field, t->position.X + 2, t->position.Y).filled;
        i += grid_at(field, t->position.X + 2, t->position.Y + 2).filled;
        return i == 3;
    }
    return 0;
}

static void lockdown(Tetrimino *t) {
    FieldGrid *grid;
    for_each_filled_grid(t, dx, dy) {
        if (t->position.Y + dy >= GAME_OVER_HEIGHT) {
            game_over();
        }
        grid = &grid_at(field, t->position.X + dx, t->position.Y + dy);
        grid->filled = 1;
    }
}

static void drop(int from) {
    int filled;
    for (int y = from + 1; y < FIELD_HEIGHT; ++y) {
        filled = 0;
        for (int x = 0; x < FIELD_WIDTH; ++x) {
            filled += grid_at(field, x, y).filled;
            set_style(&grid_at(field, x, y - 1).display_object, get_style(&grid_at(field, x, y).display_object));
            grid_at(field, x, y - 1).filled = grid_at(field, x, y).filled;
            mark_repaint(&grid_at(field, x, y).display_object);
        }
        if (filled == 0) break;
    }
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        mark_repaint(&grid_at(field, x, from).display_object);
    }
}

static int clear(int line) {
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        if (!grid_at(field, x, line).filled) {
            return 0;
        }
    }
    return 1;
}

static int do_clear(Tetrimino *t) {
    int cleared_lines = 0;
    for (int y = t->position.Y + t->type->height - 1; y >= t->position.Y; --y) {
        if (clear(y)) {
            cleared_lines += 1;
            drop(y);
        }
    }
    return cleared_lines;
}

static Position _no_move = { 0, 0 };
static int random_list[NUM_TEMPLATES];
static int random_index = NUM_TEMPLATES;
static Tetrimino *next_tetrimino() {
    if (random_index == NUM_TEMPLATES) {
        LOG("New random bag.\n");
        for (int i = 0; i < NUM_TEMPLATES; ++i) {
            random_list[i] = i;
        }
        int swap;
        int rand_id;
        for (int i = NUM_TEMPLATES - 1; i >= 0; --i) {
            rand_id = xorshift64star() % (i + 1);
            swap = random_list[i];
            random_list[i] = random_list[rand_id];
            random_list[rand_id] = swap;
        }
        random_index = 0;
    }
    int random = random_list[random_index];
    ++random_index;
    Tetrimino *next = malloc(sizeof(*next));
    LOG("Generating random number\n");
    LOG("Initializing new tetrimino: %d\n", random);
    next->type = templates[random];
    LOG("next->type %x\n", next->type);
    next->direction = 0;
    next->position = next->type->spawn_position;
    next->position.X += FIELD_WIDTH / 2;
    next->position.Y += GAME_OVER_HEIGHT + 1;
    next->visible = 1;
    next->style = next->type->style;
    LOG("Testing placement\n");
    if (!test_placement(next, &_no_move, 0)) {
        game_over();
    }
    LOG("Generated.\n");
    return next;
}

static inline void gravity_tick() {
    if (counter_countdown(&game.gravity_tick_counter)) {
        tetrimino_op(MOVE_GRAVITY);
        set_counter(&game.gravity_tick_counter, game.gravity_ticks);
    }
}

static void calculate_score(int cleared_lines, int t_spin) {
    LOG("Cleared lines %d, t_spin %d.\n", cleared_lines, t_spin);
    if (t_spin) {
        game.score.score += 300;
    }
    if (cleared_lines < 4) {
        game.score.b2b = 0;
    } else {
        if (game.score.b2b) {
            //show_special_effect("Back-To-Back!");
            game.score.score += 800;
        }
        game.score.b2b = 1;
    }
    if (cleared_lines > 0) {
        game.score.score += cleared_lines * (100 + game.score.level * 15)
                            + game.score.combo * 20 * (100 + game.score.level * 15) / 100;
        game.score.combo += 1;
    } else {
        game.score.combo = 0;
    }
}

static inline void upgrade() {
    if (game.score.level < MAX_LEVEL - 1 && game.score.score >= settings.level_up_score[game.score.level]) {
        game.score.level += 1;
        game.gravity_ticks = settings.gravity[game.score.level];
        game.lockdown_delay = settings.lockdown_delay[game.score.level];
    }
}

static inline void update_next_preview() {
    game.active_tetrimino = game.next_tetriminos[0];
    game.next_tetriminos[0] = game.next_tetriminos[1];
    game.next_tetriminos[1] = game.next_tetriminos[2];
    game.next_tetriminos[2] = next_tetrimino();
}

static inline void render_next_preview() {
    render_next_tetrimino_preview(game.next_tetriminos[0], next_preview[0]);
    render_next_tetrimino_preview(game.next_tetriminos[1], next_preview[1]);
    render_next_tetrimino_preview(game.next_tetriminos[2], next_preview[2]);
}

static void lockdown_tick() {
    if (counter_countdown(&game.lockdown_tick_counter)) {
        lockdown(game.active_tetrimino);
        repaint_tetrimino_with_style(game.dropdown_preview_tetrimino, &not_placed_style);
        repaint_tetrimino_with_style(game.active_tetrimino, game.active_tetrimino->style);
        int cleared_lines = do_clear(game.active_tetrimino);
        calculate_score(cleared_lines, is_t_spin(game.active_tetrimino));
        upgrade();
        show_score(game.score.score, game.score.level);
        free(game.active_tetrimino);
        game.active_tetrimino = NULL;
        disable_counter(&game.lockdown_tick_counter);
        set_counter(&game.gravity_tick_counter, game.gravity_ticks);
        set_counter(&game.movement_lockdown_reset_counter, settings.max_movement_lockdown_reset_counter);
        update_next_preview();
        render_next_preview();
        /* TODO: Sometimes the next preview fails to refresh. */
        /* Do not render placed block as not placed. */
        game.dropdown_preview_tetrimino->style = NULL;
        repaint_tetrimino_with_style(game.active_tetrimino, game.active_tetrimino->style);
    }
}

static void longpress_tick() {
    if (counter_countdown(&game.longpress_state.das_tick_counter)) {
        if (counter_countdown(&game.longpress_state.ar_tick_counter)) {
            tetrimino_op(game.longpress_state.op);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        }
    }
}

void keyboard_dispatch(Key key, int state) {
    LOG("Key: %d state: %d\n", key, state);
    if (key == settings.key_map.LEFT || key == settings.key_map.RIGHT || key == settings.key_map.DOWN || key == settings.key_map.HARD_DROP
            || key == settings.key_map.CLOCKWISE || key == settings.key_map.COUNTER_CLOCKWISE) {
        if (state == KEY_PRESSED) {
            delete_list_item(&game.keyboard_pressed_keys, key);
            list_insert_into_head(&game.keyboard_pressed_keys, key);
        } else {
            delete_list_item(&game.keyboard_pressed_keys, key);
            if (list_is_empty(&game.keyboard_pressed_keys)) {
                disable_counter(&game.longpress_state.das_tick_counter);
                set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
            }
        }
    } else if (key == settings.key_map.PAUSE && state == KEY_PRESSED) {
        game.paused = 1;
        disable_counter(&game.longpress_state.das_tick_counter);
        set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        show_pause_menu();
        return;
    } else {
        return;
    }
    key = get_list_head(&game.keyboard_pressed_keys);
    if (key >= 0) {
        if (key == settings.key_map.LEFT) {
            game.longpress_state.op = MOVE_LEFT;
            tetrimino_op(MOVE_LEFT);
            set_counter(&game.longpress_state.das_tick_counter, settings.das_delay);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        } else if (key == settings.key_map.RIGHT) {
            game.longpress_state.op = MOVE_RIGHT;
            tetrimino_op(MOVE_RIGHT);
            set_counter(&game.longpress_state.das_tick_counter, settings.das_delay);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        } else if (key == settings.key_map.DOWN) {
            game.longpress_state.op = MOVE_DOWN;
            tetrimino_op(MOVE_DOWN);
            set_counter(&game.longpress_state.das_tick_counter, settings.das_delay);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        } else if (key == settings.key_map.HARD_DROP) {
            tetrimino_op(MOVE_HARD_DROP);
        } else if (key == settings.key_map.CLOCKWISE) {
            game.longpress_state.op = ROTATE_CLOCKWISE;
            tetrimino_op(ROTATE_CLOCKWISE);
            set_counter(&game.longpress_state.das_tick_counter, settings.das_delay);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        } else if (key == settings.key_map.COUNTER_CLOCKWISE) {
            game.longpress_state.op = ROTATE_COUNTER_CLOCKWISE;
            tetrimino_op(ROTATE_COUNTER_CLOCKWISE);
            set_counter(&game.longpress_state.das_tick_counter, settings.das_delay);
            set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
        }
    } else {
        disable_counter(&game.longpress_state.das_tick_counter);
        set_counter(&game.longpress_state.ar_tick_counter, settings.ar_ticks);
    }
}

void tick() {
    //while(!game.over) {
        if (!game.paused && !game.over) {
            longpress_tick();
            gravity_tick();
            lockdown_tick();
        }
        //usleep(MSPT * 1000);
    //}
}

void init_field() {
    for (int x = 0; x < FIELD_WIDTH; ++x) {
        for (int y = 0; y < FIELD_HEIGHT; ++y) {
            grid_at(field, x, y).filled = 0;
            set_style(&grid_at(field, x, y).display_object, &not_placed_style);
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int x = 0; x < NEXT_PREVIEW_WIDTH; ++x) {
            for (int y = 0; y < NEXT_PREVIEW_HEIGHT; ++y) {
                set_style(&grid_at(next_preview[i], x, y).display_object, &not_placed_style);
            }
        }
    }
    LOG("Initialized field.\n");
    game.over = -1;
}

void start_game() {
    if (game.over > 0) {
        reset_game();
    }
    game.gravity_ticks = settings.gravity[0];
    if (settings.lockdown_strategy == LOCKDOWN_CLASSIC) {
        game.lockdown_delay = 0;
    } else {
        game.lockdown_delay = settings.lockdown_delay[0];
    }
    game.active_tetrimino = next_tetrimino();
    game.next_tetriminos[0] = next_tetrimino();
    game.next_tetriminos[1] = next_tetrimino();
    game.next_tetriminos[2] = next_tetrimino();
    game.dropdown_preview_tetrimino = malloc(sizeof(*game.dropdown_preview_tetrimino));
    game.paused = 0;
    game.over = 0;
    game.score.score = 0;
    game.score.level = 0;
    game.score.b2b = 0;
    game.score.combo = 0;
    LOG("Initialized game status.\n");
    set_counter(&game.gravity_tick_counter, game.gravity_ticks);
    set_counter(&game.movement_lockdown_reset_counter, settings.max_movement_lockdown_reset_counter);
    disable_counter(&game.lockdown_tick_counter);
    disable_counter(&game.longpress_state.das_tick_counter);
    init_list(&game.keyboard_pressed_keys);
    LOG("Initialized counters.\n");
    repaint_tetrimino_with_style(game.active_tetrimino, game.active_tetrimino->style);
    show_score(0, 0);
    render_next_preview();
    LOG("Initialized initial displays.\n");
    //pthread_t tid;
    //pthread_create(&tid, NULL, tick, NULL);
}

void resume_game() {
    game.paused = 0;
}