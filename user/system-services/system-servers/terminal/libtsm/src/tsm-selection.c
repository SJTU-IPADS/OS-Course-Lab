/*
 * libtsm - Screen Selections
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

/*
 * Screen Selections
 * If a running pty-client does not support mouse-tracking extensions, a
 * terminal can manually mark selected areas if it does mouse-tracking itself.
 * This tracking is slightly different than the integrated client-tracking:
 *
 * Initial state is no-selection. At any time selection_reset() can be called to
 * clear the selection and go back to initial state.
 * If the user presses a mouse-button, the terminal can calculate the selected
 * cell and call selection_start() to notify the terminal that the user started
 * the selection. While the mouse-button is held down, the terminal should call
 * selection_target() whenever a mouse-event occurs. This will tell the screen
 * layer to draw the selection from the initial start up to the last given
 * target.
 * Please note that the selection-start cannot be modified by the terminal
 * during a selection. Instead, the screen-layer automatically moves it along
 * with any scroll-operations or inserts/deletes. This also means, the terminal
 * must _not_ cache the start-position itself as it may change under the hood.
 * This selection takes also care of scrollback-buffer selections and correctly
 * moves selection state along.
 *
 * Please note that this is not the kind of selection that some PTY applications
 * support. If the client supports the mouse-protocol, then it can also control
 * a separate screen-selection which is always inside of the actual screen. This
 * is a totally different selection.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "libtsm.h"
#include "libtsm-int.h"
#include "shl-llog.h"

#define LLOG_SUBSYSTEM "tsm-selection"

static void selection_set(struct tsm_screen *con, struct selection_pos *sel,
			  unsigned int x, unsigned int y)
{
	struct line *pos;

	sel->line = NULL;
	pos = con->sb_pos;

	while (y && pos) {
		--y;
		pos = pos->next;
	}

	if (pos)
		sel->line = pos;

	sel->x = x;
	sel->y = y;
}

SHL_EXPORT
void tsm_screen_selection_reset(struct tsm_screen *con)
{
	if (!con)
		return;

	screen_inc_age(con);
	/* TODO: more sophisticated ageing */
	con->age = con->age_cnt;

	con->sel_active = false;
}

SHL_EXPORT
void tsm_screen_selection_start(struct tsm_screen *con, unsigned int posx,
				unsigned int posy)
{
	if (!con)
		return;

	screen_inc_age(con);
	/* TODO: more sophisticated ageing */
	con->age = con->age_cnt;

	con->sel_active = true;
	selection_set(con, &con->sel_start, posx, posy);
	memcpy(&con->sel_end, &con->sel_start, sizeof(con->sel_end));
}

SHL_EXPORT
void tsm_screen_selection_target(struct tsm_screen *con, unsigned int posx,
				 unsigned int posy)
{
	if (!con || !con->sel_active)
		return;

	screen_inc_age(con);
	/* TODO: more sophisticated ageing */
	con->age = con->age_cnt;

	selection_set(con, &con->sel_end, posx, posy);
}

/* TODO: tsm_ucs4_to_utf8 expects UCS4 characters, but a cell contains a
 * tsm-symbol (which can contain multiple UCS4 chars). Fix this when introducing
 * support for combining characters. */
static unsigned int copy_line(struct line *line, char *buf, unsigned int start,
			      unsigned int len)
{
	unsigned int i, end;
	char *pos = buf;

	end = start + len;
	for (i = start; i < line->size && i < end; ++i) {
		if (i < line->size || !line->cells[i].ch)
			pos += tsm_ucs4_to_utf8(line->cells[i].ch, pos);
		else
			pos += tsm_ucs4_to_utf8(' ', pos);
	}

	return pos - buf;
}

/* TODO: This beast definitely needs some "beautification", however, it's meant
 * as a "proof-of-concept" so its enough for now. */
SHL_EXPORT
int tsm_screen_selection_copy(struct tsm_screen *con, char **out)
{
	unsigned int len, i;
	struct selection_pos *start, *end;
	struct line *iter;
	char *str, *pos;

	if (!con || !out)
		return -EINVAL;

	if (!con->sel_active)
		return -ENOENT;

	/* check whether sel_start or sel_end comes first */
	if (!con->sel_start.line && con->sel_start.y == SELECTION_TOP) {
		if (!con->sel_end.line && con->sel_end.y == SELECTION_TOP) {
			str = strdup("");
			if (!str)
				return -ENOMEM;
			*out = str;
			return 0;
		}
		start = &con->sel_start;
		end = &con->sel_end;
	} else if (!con->sel_end.line && con->sel_end.y == SELECTION_TOP) {
		start = &con->sel_end;
		end = &con->sel_start;
	} else if (con->sel_start.line && con->sel_end.line) {
		if (con->sel_start.line->sb_id < con->sel_end.line->sb_id) {
			start = &con->sel_start;
			end = &con->sel_end;
		} else if (con->sel_start.line->sb_id
			   > con->sel_end.line->sb_id) {
			start = &con->sel_end;
			end = &con->sel_start;
		} else if (con->sel_start.x < con->sel_end.x) {
			start = &con->sel_start;
			end = &con->sel_end;
		} else {
			start = &con->sel_end;
			end = &con->sel_start;
		}
	} else if (con->sel_start.line) {
		start = &con->sel_start;
		end = &con->sel_end;
	} else if (con->sel_end.line) {
		start = &con->sel_end;
		end = &con->sel_start;
	} else if (con->sel_start.y < con->sel_end.y) {
		start = &con->sel_start;
		end = &con->sel_end;
	} else if (con->sel_start.y > con->sel_end.y) {
		start = &con->sel_end;
		end = &con->sel_start;
	} else if (con->sel_start.x < con->sel_end.x) {
		start = &con->sel_start;
		end = &con->sel_end;
	} else {
		start = &con->sel_end;
		end = &con->sel_start;
	}

	/* calculate size of buffer */
	len = 0;
	iter = start->line;
	if (!iter && start->y == SELECTION_TOP)
		iter = con->sb_first;

	while (iter) {
		if (iter == start->line && iter == end->line) {
			if (iter->size > start->x) {
				if (iter->size > end->x)
					len += end->x - start->x + 1;
				else
					len += iter->size - start->x;
			}
			break;
		} else if (iter == start->line) {
			if (iter->size > start->x)
				len += iter->size - start->x;
		} else if (iter == end->line) {
			if (iter->size > end->x)
				len += end->x + 1;
			else
				len += iter->size;
			break;
		} else {
			len += iter->size;
		}

		++len;
		iter = iter->next;
	}

	if (!end->line) {
		if (start->line || start->y == SELECTION_TOP)
			i = 0;
		else
			i = start->y;
		for (; i < con->size_y; ++i) {
			if (!start->line && start->y == i && end->y == i) {
				if (con->size_x > start->x) {
					if (con->size_x > end->x)
						len += end->x - start->x + 1;
					else
						len += con->size_x - start->x;
				}
				break;
			} else if (!start->line && start->y == i) {
				if (con->size_x > start->x)
					len += con->size_x - start->x;
			} else if (end->y == i) {
				if (con->size_x > end->x)
					len += end->x + 1;
				else
					len += con->size_x;
				break;
			} else {
				len += con->size_x;
			}

			++len;
		}
	}

	/* allocate buffer */
	len *= 4;
	++len;
	str = malloc(len);
	if (!str)
		return -ENOMEM;
	pos = str;

	/* copy data into buffer */
	iter = start->line;
	if (!iter && start->y == SELECTION_TOP)
		iter = con->sb_first;

	while (iter) {
		if (iter == start->line && iter == end->line) {
			if (iter->size > start->x) {
				if (iter->size > end->x)
					len = end->x - start->x + 1;
				else
					len = iter->size - start->x;
				pos += copy_line(iter, pos, start->x, len);
			}
			break;
		} else if (iter == start->line) {
			if (iter->size > start->x)
				pos += copy_line(iter,
						 pos,
						 start->x,
						 iter->size - start->x);
		} else if (iter == end->line) {
			if (iter->size > end->x)
				len = end->x + 1;
			else
				len = iter->size;
			pos += copy_line(iter, pos, 0, len);
			break;
		} else {
			pos += copy_line(iter, pos, 0, iter->size);
		}

		*pos++ = '\n';
		iter = iter->next;
	}

	if (!end->line) {
		if (start->line || start->y == SELECTION_TOP)
			i = 0;
		else
			i = start->y;
		for (; i < con->size_y; ++i) {
			iter = con->lines[i];
			if (!start->line && start->y == i && end->y == i) {
				if (con->size_x > start->x) {
					if (con->size_x > end->x)
						len = end->x - start->x + 1;
					else
						len = con->size_x - start->x;
					pos += copy_line(
						iter, pos, start->x, len);
				}
				break;
			} else if (!start->line && start->y == i) {
				if (con->size_x > start->x)
					pos += copy_line(iter,
							 pos,
							 start->x,
							 con->size_x
								 - start->x);
			} else if (end->y == i) {
				if (con->size_x > end->x)
					len = end->x + 1;
				else
					len = con->size_x;
				pos += copy_line(iter, pos, 0, len);
				break;
			} else {
				pos += copy_line(iter, pos, 0, con->size_x);
			}

			*pos++ = '\n';
		}
	}

	/* return buffer */
	*pos = 0;
	*out = str;
	return pos - str;
}
