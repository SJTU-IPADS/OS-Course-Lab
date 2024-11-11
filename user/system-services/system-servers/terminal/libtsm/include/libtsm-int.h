/*
 * TSM - Main internal header
 *
 * Copyright (c) 2011-2013 David Herrmann <dh.herrmann@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef TSM_LIBTSM_INT_H
#define TSM_LIBTSM_INT_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include "libtsm.h"
#include "shl-llog.h"

#define SHL_EXPORT __attribute__((visibility("default")))

/* max combined-symbol length */
#define TSM_UCS4_MAXLEN 10

/* symbols */

struct tsm_symbol_table;

extern const tsm_symbol_t tsm_symbol_default;

int tsm_symbol_table_new(struct tsm_symbol_table **out);
void tsm_symbol_table_ref(struct tsm_symbol_table *tbl);
void tsm_symbol_table_unref(struct tsm_symbol_table *tbl);

tsm_symbol_t tsm_symbol_make(uint32_t ucs4);
tsm_symbol_t tsm_symbol_append(struct tsm_symbol_table *tbl, tsm_symbol_t sym,
			       uint32_t ucs4);
const uint32_t *tsm_symbol_get(struct tsm_symbol_table *tbl, tsm_symbol_t *sym,
			       size_t *size);
unsigned int tsm_symbol_get_width(struct tsm_symbol_table *tbl,
				  tsm_symbol_t sym);

/* utf8 state machine */

struct tsm_utf8_mach;

enum tsm_utf8_mach_state {
	TSM_UTF8_START,
	TSM_UTF8_ACCEPT,
	TSM_UTF8_REJECT,
	TSM_UTF8_EXPECT1,
	TSM_UTF8_EXPECT2,
	TSM_UTF8_EXPECT3,
};

int tsm_utf8_mach_new(struct tsm_utf8_mach **out);
void tsm_utf8_mach_free(struct tsm_utf8_mach *mach);

int tsm_utf8_mach_feed(struct tsm_utf8_mach *mach, char c);
uint32_t tsm_utf8_mach_get(struct tsm_utf8_mach *mach);
void tsm_utf8_mach_reset(struct tsm_utf8_mach *mach);

/* TSM screen */

struct cell {
	tsm_symbol_t ch; /* stored character */
	unsigned int width; /* character width */
	struct tsm_screen_attr attr; /* cell attributes */
	tsm_age_t age; /* age of the single cell */
};

struct line {
	struct line *next; /* next line (NULL if not sb) */
	struct line *prev; /* prev line (NULL if not sb) */

	unsigned int size; /* real width */
	struct cell *cells; /* actuall cells */
	uint64_t sb_id; /* sb ID */
	tsm_age_t age; /* age of the whole line */
};

#define SELECTION_TOP -1
struct selection_pos {
	struct line *line;
	unsigned int x;
	int y;
};

struct tsm_screen {
	size_t ref;
	llog_submit_t llog;
	void *llog_data;
	unsigned int opts;
	unsigned int flags;
	struct tsm_symbol_table *sym_table;

	/* default attributes for new cells */
	struct tsm_screen_attr def_attr;

	/* ageing */
	tsm_age_t age_cnt; /* current age counter */
	unsigned int age_reset : 1; /* age-overflow flag */

	/* current buffer */
	unsigned int size_x; /* width of screen */
	unsigned int size_y; /* height of screen */
	unsigned int margin_top; /* top-margin index */
	unsigned int margin_bottom; /* bottom-margin index */
	unsigned int line_num; /* real number of allocated lines */
	struct line **lines; /* active lines; copy of main/alt */
	struct line **main_lines; /* real main lines */
	struct line **alt_lines; /* real alternative lines */
	tsm_age_t age; /* whole screen age */

	/* scroll-back buffer */
	unsigned int sb_count; /* number of lines in sb */
	struct line *sb_first; /* first line; was moved first */
	struct line *sb_last; /* last line; was moved last*/
	unsigned int sb_max; /* max-limit of lines in sb */
	struct line *sb_pos; /* current position in sb or NULL */
	uint64_t sb_last_id; /* last id given to sb-line */

	/* cursor: positions are always in-bound, but cursor_x might be
	 * bigger than size_x if new-line is pending */
	unsigned int cursor_x; /* current cursor x-pos */
	unsigned int cursor_y; /* current cursor y-pos */

	/* tab ruler */
	bool *tab_ruler; /* tab-flag for all cells of one row */

	/* selection */
	bool sel_active;
	struct selection_pos sel_start;
	struct selection_pos sel_end;
};

void screen_cell_init(struct tsm_screen *con, struct cell *cell);

void tsm_screen_set_opts(struct tsm_screen *scr, unsigned int opts);
void tsm_screen_reset_opts(struct tsm_screen *scr, unsigned int opts);
unsigned int tsm_screen_get_opts(struct tsm_screen *scr);

static inline void screen_inc_age(struct tsm_screen *con)
{
	if (!++con->age_cnt) {
		con->age_reset = 1;
		++con->age_cnt;
	}
}

/* available character sets */

typedef tsm_symbol_t tsm_vte_charset[96];

extern tsm_vte_charset tsm_vte_unicode_lower;
extern tsm_vte_charset tsm_vte_unicode_upper;
extern tsm_vte_charset tsm_vte_dec_supplemental_graphics;
extern tsm_vte_charset tsm_vte_dec_special_graphics;

#endif /* TSM_LIBTSM_INT_H */
