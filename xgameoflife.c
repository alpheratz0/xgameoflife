/*
	Copyright (C) 2022-2026 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License version 2 as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
	more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc., 59
	Temple Place, Suite 330, Boston, MA 02111-1307 USA

	 __________
	( the game )
	 ----------
	    o
	     o  /\/\
	       \   /
	       |  0 >>
	       |___|
	 __((_<|   |
	(          |
	(__________)
	   |      |
	   |      |
	   /\     /\

*/

#define _XOPEN_SOURCE 500

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xkb.h>
#include <xcb/xproto.h>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "config.h"

#define UNUSED __attribute__((unused))

#define NANOSECONDS_PER_SECOND (1000*1000*1000)
#define FONT_HEIGHT 8
#define INFO_BAR_HEIGHT 20

#define XGAMEOFLIFE_WM_NAME "xgameoflife"
#define XGAMEOFLIFE_WM_CLASS "xgameoflife\0xgameoflife\0"

enum {
	GC_ALIVE,
	GC_DEAD,
	GC_BORDER,
	GC_TEXT,
	GC_BAR,
	GC_COUNT
};

enum {
	CURSOR_FLEUR,
	CURSOR_LEFT_PTR,
	CURSOR_COUNT
};

/* x11 */
static xcb_connection_t *conn;
static xcb_window_t window;
static xcb_pixmap_t backbuf;
static uint8_t depth;
static int16_t width, height;
static xcb_atom_t wm_delete_window;
static xcb_cursor_context_t *cctx;
static xcb_key_symbols_t *ksyms;
static xcb_cursor_t cursors[CURSOR_COUNT];
static xcb_gcontext_t graphics[GC_COUNT];

/* board */
static int32_t rows;
static int32_t columns;
static int32_t cellsize;
static uint8_t *cells[2];

/* game state */
static int running, was_running;
static int should_quit;
static int gps;
static xcb_point_t hovered;

/* dragging */
static int dragging;
static xcb_point_t offset;
static xcb_point_t mousepos;

/* blocksleep */
static struct timespec begin_ts;

static void
die(const char *fmt, ...)
{
	va_list args;

	fputs("xgameoflife: ", stderr);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

static void *
xcalloc(size_t n, size_t size)
{
	void *p;

	if (NULL == (p = calloc(n, size)))
		die("error while calling calloc, no memory available");

	return p;
}

static void
blockstart(void)
{
	clock_gettime(CLOCK_MONOTONIC, &begin_ts);
}

static void
blockwait(int nanoseconds)
{
	struct timespec now_ts, delta_ts, end_ts;

	end_ts = begin_ts;
	end_ts.tv_nsec += nanoseconds;

	if ((end_ts.tv_nsec / NANOSECONDS_PER_SECOND) > 0) {
		end_ts.tv_sec += end_ts.tv_nsec / NANOSECONDS_PER_SECOND;
		end_ts.tv_nsec = end_ts.tv_nsec % NANOSECONDS_PER_SECOND;
	}

	clock_gettime(CLOCK_MONOTONIC, &now_ts);

	/* check if we already reached the end */
	if (now_ts.tv_sec > end_ts.tv_sec)
		return;

	if (now_ts.tv_sec == end_ts.tv_sec && now_ts.tv_nsec >= end_ts.tv_nsec)
		return;

	delta_ts.tv_sec = end_ts.tv_sec - now_ts.tv_sec;
	delta_ts.tv_nsec = end_ts.tv_nsec - now_ts.tv_nsec;

	if (delta_ts.tv_nsec < 0) {
		delta_ts.tv_nsec += NANOSECONDS_PER_SECOND;
		--delta_ts.tv_sec;
	}

	nanosleep(&delta_ts, 0);
}

static xcb_atom_t
xatom(const char *name)
{
	xcb_atom_t atom;
	xcb_generic_error_t *error;
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply;

	cookie = xcb_intern_atom(conn, 0, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, &error);

	if (NULL != error)
		die("xcb_intern_atom failed with error code: %d",
				(int)(error->error_code));

	atom = reply->atom;
	free(reply);

	return atom;
}

static xcb_gcontext_t
xcolor(uint32_t c)
{
	xcb_gcontext_t id;
	uint32_t mask;
	uint32_t values[2];

	mask = 0;
	id = xcb_generate_id(conn);

	mask |= XCB_GC_FOREGROUND; values[0] = c;
	mask |= XCB_GC_GRAPHICS_EXPOSURES; values[1] = 0;

	xcb_create_gc(conn, id, window, mask, values);

	return id;
}

static xcb_gcontext_t
xfont(const char *name, uint32_t foreground, uint32_t background)
{
	xcb_font_t font;
	xcb_gcontext_t id;
	xcb_void_cookie_t cookie;
	uint32_t mask;
	uint32_t values[3];

	font = xcb_generate_id(conn);
	id = xcb_generate_id(conn);
	cookie = xcb_open_font_checked(conn, font, strlen(name), name);
	mask = 0;

	if (xcb_request_check(conn, cookie))
		die("font not found: %s", name);

	mask |= XCB_GC_FOREGROUND; values[0] = foreground;
	mask |= XCB_GC_BACKGROUND; values[1] = background;
	mask |= XCB_GC_FONT;       values[2] = font;

	xcb_create_gc(conn, id, window, mask, values);
	xcb_close_font(conn, font);

	return id;
}

static void
create_window(void)
{
	xcb_screen_t *screen;

	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL)))
		die("can't open display");

	if (NULL == (screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data))
		die("can't get default screen");

	if (xcb_cursor_context_new(conn, screen, &cctx) != 0)
		die("can't create cursor context");

	ksyms = xcb_key_symbols_alloc(conn);
	window = xcb_generate_id(conn);
	depth = screen->root_depth;
	width = 800;
	height = 600;

	xcb_create_window_aux(
		conn, XCB_COPY_FROM_PARENT, window, screen->root,
		0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(const xcb_create_window_value_list_t []) {{
			.background_pixel = dead_color,
			.event_mask = XCB_EVENT_MASK_EXPOSURE |
			              XCB_EVENT_MASK_KEY_PRESS |
			              XCB_EVENT_MASK_KEY_RELEASE |
			              XCB_EVENT_MASK_BUTTON_PRESS |
			              XCB_EVENT_MASK_BUTTON_RELEASE |
			              XCB_EVENT_MASK_POINTER_MOTION |
			              XCB_EVENT_MASK_STRUCTURE_NOTIFY
		}}
	);

	/* all rendering goes to an off-screen pixmap that is
	 * then copied to the window in a single operation,
	 * this avoids flickering */
	backbuf = xcb_generate_id(conn);
	xcb_create_pixmap(conn, depth, backbuf, window, width, height);

	/* set WM_NAME */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING, 8, sizeof(XGAMEOFLIFE_WM_NAME) - 1,
		XGAMEOFLIFE_WM_NAME
	);

	/* set WM_CLASS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS,
		XCB_ATOM_STRING, 8, sizeof(XGAMEOFLIFE_WM_CLASS) - 1,
		XGAMEOFLIFE_WM_CLASS
	);

	/* add WM_DELETE_WINDOW to WM_PROTOCOLS */
	wm_delete_window = xatom("WM_DELETE_WINDOW");

	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window, xatom("WM_PROTOCOLS"),
		XCB_ATOM_ATOM, 32, 1, &wm_delete_window
	);

	/* load graphics */
	graphics[GC_DEAD] = xcolor(dead_color);
	graphics[GC_ALIVE] = xcolor(alive_color);
	graphics[GC_BORDER] = xcolor(border_color);
	graphics[GC_TEXT] = xfont("fixed", text_color, bar_color);
	graphics[GC_BAR] = xcolor(bar_color);

	/* load cursors */
	cursors[CURSOR_FLEUR] = xcb_cursor_load_cursor(cctx, "fleur");
	cursors[CURSOR_LEFT_PTR] = xcb_cursor_load_cursor(cctx, "left_ptr");

	xcb_xkb_use_extension(conn, XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);

	xcb_xkb_per_client_flags(
		conn, XCB_XKB_ID_USE_CORE_KBD,
		XCB_XKB_PER_CLIENT_FLAG_DETECTABLE_AUTO_REPEAT, 1, 0, 0, 0
	);

	xcb_map_window(conn, window);
	xcb_flush(conn);
}

static void
destroy_window(void)
{
	size_t i;

	for (i = 0; i < GC_COUNT; ++i)
		xcb_free_gc(conn, graphics[i]);

	for (i = 0; i < CURSOR_COUNT; ++i)
		xcb_free_cursor(conn, cursors[i]);

	xcb_free_pixmap(conn, backbuf);
	xcb_key_symbols_free(ksyms);
	xcb_cursor_context_free(cctx);
	xcb_disconnect(conn);
}

static void
create_board(int32_t c, int32_t r)
{
	cells[0] = xcalloc(c*r, sizeof(uint8_t));
	cells[1] = xcalloc(c*r, sizeof(uint8_t));

	columns = c;
	rows = r;
	cellsize = initial_cellsize;
}

static uint8_t
get_cell(int x, int y)
{
	x = ((x % columns) + columns) % columns;
	y = ((y % rows) + rows) % rows;
	return cells[0][y * columns + x];
}

static void
set_cell(int x, int y, uint8_t value)
{
	x = ((x % columns) + columns) % columns;
	y = ((y % rows) + rows) % rows;
	cells[0][y * columns + x] = value;
}

static void
toggle_cell(int x, int y)
{
	x = ((x % columns) + columns) % columns;
	y = ((y % rows) + rows) % rows;
	cells[0][y * columns + x] ^= 1;
}

static void
advance_to_next_generation(void)
{
	int x, y, xl, xr, n;
	uint8_t *up, *mid, *down, *next, *tmp;

	for (y = 0; y < rows; ++y) {
		up = &cells[0][((y + rows - 1) % rows) * columns];
		mid = &cells[0][y * columns];
		down = &cells[0][((y + 1) % rows) * columns];
		next = &cells[1][y * columns];

		for (x = 0; x < columns; ++x) {
			xl = x == 0 ? columns - 1 : x - 1;
			xr = x == columns - 1 ? 0 : x + 1;

			n = up[xl] + up[x] + up[xr] +
			    mid[xl] + mid[xr] +
			    down[xl] + down[x] + down[xr];

			next[x] = n == 3 || (mid[x] && n == 2);
		}
	}

	/* swap */
	tmp = cells[0];
	cells[0] = cells[1];
	cells[1] = tmp;
}

static void
save_board(void)
{
	int x, y;
	struct tm *now;
	char filename[19];
	FILE *fp;

	now = localtime((const time_t[]) { time(NULL) });
	strftime(filename, sizeof(filename), "%Y%m%d%H%M%S.xg", now);

	if (NULL == (fp = fopen(filename, "w")))
		die("failed to open file %s: %s", filename, strerror(errno));

	fprintf(fp, "%dx%d\n", columns, rows);

	for (x = 0; x < columns; ++x)
		for (y = 0; y < rows; ++y)
			if (get_cell(x, y))
				fprintf(fp, "%d,%d\n", x, y);

	fclose(fp);
}

static void
load_board_xg(FILE *fp)
{
	int x, y, c, r;
	char header[64];

	if (NULL == fgets(header, sizeof(header), fp)) {
		create_board(default_columns, default_rows);
		return;
	}

	if (sscanf(header, "%dx%d", &c, &r) == 2) {
		if (c < 1 || r < 1 ||
				c > max_board_dimension || r > max_board_dimension)
			die("invalid board dimensions: %dx%d", c, r);

		create_board(c, r);
	} else {
		create_board(default_columns, default_rows);

		/* the first line was a cell, not a header */
		if (sscanf(header, "%d,%d", &x, &y) == 2)
			set_cell(x, y, 1);
	}

	while (fscanf(fp, "%d,%d\n", &x, &y) == 2)
		set_cell(x, y, 1);
}

static void
load_board_rle(FILE *fp)
{
	int c, w, h, x, y, ox, oy, run;
	char line[1024];

	/* skip comments and blank lines */
	for (;;) {
		c = fgetc(fp);

		if (c == '#') {
			if (NULL == fgets(line, sizeof(line), fp))
				die("invalid rle file: missing header");
		} else if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
			continue;
		} else {
			ungetc(c, fp);
			break;
		}
	}

	if (fscanf(fp, "x = %d, y = %d", &w, &h) != 2)
		die("invalid rle file: bad header");

	if (w < 1 || h < 1 || w > max_board_dimension || h > max_board_dimension)
		die("invalid rle file: bad pattern size: %dx%d", w, h);

	/* skip the rest of the header line (rule, if any) */
	if (NULL == fgets(line, sizeof(line), fp))
		die("invalid rle file: missing pattern");

	create_board(
		w > default_columns ? w : default_columns,
		h > default_rows ? h : default_rows
	);

	/* center the pattern in the initial viewport */
	ox = (width / cellsize - w) / 2;
	oy = (height / cellsize - h) / 2;

	if (ox < 0) ox = 0;
	if (oy < 0) oy = 0;

	x = y = run = 0;

	while ((c = fgetc(fp)) != EOF && c != '!') {
		if (isdigit(c)) {
			run = run * 10 + (c - '0');
		} else if (c == 'b' || c == 'o') {
			for (run = run > 0 ? run : 1; run > 0; --run, ++x)
				if (c == 'o')
					set_cell(ox + x, oy + y, 1);
		} else if (c == '$') {
			y += run > 0 ? run : 1;
			x = run = 0;
		} else if (!isspace(c)) {
			die("invalid rle file: unexpected character: %c", c);
		}
	}
}

static void
load_board(const char *path)
{
	int c;
	FILE *fp;

	if (strcmp(path, "-") == 0) {
		fp = stdin;
	} else if (NULL == (fp = fopen(path, "r"))) {
		die("failed to open file %s: %s", path, strerror(errno));
	}

	/* rle files start with a comment or the "x = ..." header */
	c = fgetc(fp);
	ungetc(c, fp);

	if (c == '#' || c == 'x')
		load_board_rle(fp);
	else
		load_board_xg(fp);

	if (fp != stdin)
		fclose(fp);
}

static void
destroy_board(void)
{
	free(cells[0]);
	free(cells[1]);
}

static void
render_scene(void)
{
	int32_t col, row;
	int32_t vcolumns, vrows;
	int32_t coloff, rowoff;
	int32_t celloffx, celloffy;

	int32_t rectc;
	static xcb_rectangle_t rects[8192];

	int32_t i;
	xcb_point_t line[2];

	xcb_rectangle_t box;
	char text[200];

	rectc = 0;

	box.x = box.y = 0;
	box.width = width;
	box.height = height;

	xcb_poly_fill_rectangle(conn, backbuf, graphics[GC_DEAD], 1, &box);

	/* full visible columns & rows */
	vcolumns = width / cellsize;
	vrows = height / cellsize;

	/* column & row offset */
	coloff = offset.x / cellsize;
	rowoff = offset.y / cellsize;

	/* cell x & y offset */
	celloffx = offset.x % cellsize;
	celloffy = offset.y % cellsize;

	for (col = -1; col < vcolumns + 1; ++col) {
		for (row = -1; row < vrows + 1; ++row) {
			if (get_cell(col - coloff, row - rowoff)) {
				rects[rectc].x = col * cellsize + celloffx;
				rects[rectc].y = row * cellsize + celloffy;
				rects[rectc].width = rects[rectc].height = cellsize;

				if (++rectc == (sizeof(rects) / sizeof(rects[0]))) {
					xcb_poly_fill_rectangle(
						conn, backbuf, graphics[GC_ALIVE],
						rectc, rects
					);

					rectc = 0;
				}
			}
		}
	}

	if (rectc != 0)
		xcb_poly_fill_rectangle(conn, backbuf, graphics[GC_ALIVE], rectc, rects);

#define DRAW_LINE(x0,y0,x1,y1) do {                             \
	line[0].x = (x0); line[0].y = (y0);                         \
	line[1].x = (x1); line[1].y = (y1);                         \
	xcb_poly_line(                                              \
		conn, XCB_COORD_MODE_ORIGIN, backbuf,                   \
		graphics[GC_BORDER], 2, line                            \
	);                                                          \
} while (0)

	for (i = -1; i < vcolumns + 1; ++i)
		DRAW_LINE(i*cellsize+celloffx, 0, i*cellsize+celloffx, height);

	for (i = -1; i < vrows + 1; ++i)
		DRAW_LINE(0, i*cellsize+celloffy, width, i*cellsize+celloffy);

#undef DRAW_LINE

	if (running)
		snprintf(text, sizeof(text), "* RUNNING (%d gen/s)", gps);
	else
		snprintf(text, sizeof(text), "* PAUSED (%hd, %hd) (%d gen/s)",
				hovered.x, hovered.y, gps);

	box.x = 0;
	box.y = height - INFO_BAR_HEIGHT;
	box.width = width;
	box.height = INFO_BAR_HEIGHT;

	xcb_poly_fill_rectangle(conn, backbuf, graphics[GC_BAR], 1, &box);

	xcb_image_text_8(
		conn, strlen(text), backbuf, graphics[GC_TEXT], INFO_BAR_HEIGHT / 2,
		height - (INFO_BAR_HEIGHT - FONT_HEIGHT) / 2, text
	);

	xcb_copy_area(conn, backbuf, window, graphics[GC_DEAD],
			0, 0, 0, 0, width, height);

	xcb_flush(conn);
}

static void
usage(void)
{
	puts("usage: xgameoflife [-hv] [file]");
	exit(0);
}

static void
version(void)
{
	puts("xgameoflife version "VERSION);
	exit(0);
}

static void
h_client_message(xcb_client_message_event_t *ev)
{
	/* check if the wm sent a delete window message */
	/* https://www.x.org/docs/ICCCM/icccm.pdf */
	if (ev->data.data32[0] == wm_delete_window)
		should_quit = 1;
}

static void
h_configure_notify(xcb_configure_notify_event_t *ev)
{
	if (ev->width == width && ev->height == height)
		return;

	width = ev->width;
	height = ev->height;

	xcb_free_pixmap(conn, backbuf);
	backbuf = xcb_generate_id(conn);
	xcb_create_pixmap(conn, depth, backbuf, window, width, height);

	render_scene();
}

static void
h_expose(UNUSED xcb_expose_event_t *ev)
{
	render_scene();
}

static void
h_key_press(xcb_key_press_event_t *ev)
{
	xcb_keysym_t key;

	key = xcb_key_symbols_get_keysym(ksyms, ev->detail, 0);

	switch (key) {
	case XKB_KEY_space:
		if (!dragging) {
			running = !running;
			render_scene();
		}
		break;
	case XKB_KEY_n:
		if (!running) {
			advance_to_next_generation();
			render_scene();
		}
		break;
	case XKB_KEY_c:
		if (!running) {
			memset(cells[0], 0, columns * rows);
			render_scene();
		}
		break;
	case XKB_KEY_plus:
	case XKB_KEY_equal:
		if (gps < max_generations_per_second) {
			++gps;
			render_scene();
		}
		break;
	case XKB_KEY_minus:
		if (gps > 1) {
			--gps;
			render_scene();
		}
		break;
	case XKB_KEY_q:
	case XKB_KEY_Escape:
		should_quit = 1;
		break;
	}
}

static void
h_key_release(xcb_key_release_event_t *ev)
{
	xcb_keysym_t key;

	key = xcb_key_symbols_get_keysym(ksyms, ev->detail, 0);

	if (!running && key == XKB_KEY_s && ev->state & XCB_MOD_MASK_CONTROL)
		save_board();
}

static void
h_button_press(xcb_button_press_event_t *ev)
{
	int16_t zoom;

	if (running && ev->detail == XCB_BUTTON_INDEX_2)
		was_running = 1;

	running = 0;
	zoom = 0;

	switch (ev->detail) {
	case XCB_BUTTON_INDEX_1:
		toggle_cell(hovered.x, hovered.y);
		render_scene();
		break;
	case XCB_BUTTON_INDEX_2:
		dragging = 1;
		mousepos.x = ev->event_x;
		mousepos.y = ev->event_y;
		xcb_change_window_attributes(conn, window, XCB_CW_CURSOR, &cursors[CURSOR_FLEUR]);
		xcb_flush(conn);
		break;
	case XCB_BUTTON_INDEX_4:
		if (cellsize < max_cellsize)
			zoom = 1;
		break;
	case XCB_BUTTON_INDEX_5:
		if (cellsize > min_cellsize)
			zoom = -1;
		break;
	}

	if (zoom) {
		/* keep the cell under the pointer fixed while zooming */
		offset.x = (offset.x * (cellsize + zoom) - zoom * ev->event_x) / cellsize;
		offset.y = (offset.y * (cellsize + zoom) - zoom * ev->event_y) / cellsize;
		cellsize += zoom;
		render_scene();
	}
}

static void
h_button_release(xcb_button_release_event_t *ev)
{
	if (ev->detail == XCB_BUTTON_INDEX_2) {
		if (was_running) running = 1;
		was_running = 0;
		dragging = 0;
		xcb_change_window_attributes(conn, window, XCB_CW_CURSOR, &cursors[CURSOR_LEFT_PTR]);
		xcb_flush(conn);
	}
}

static void
h_motion_notify(xcb_motion_notify_event_t *ev)
{
	if (dragging) {
		offset.x += ev->event_x - mousepos.x;
		offset.y += ev->event_y - mousepos.y;
		mousepos.x = ev->event_x;
		mousepos.y = ev->event_y;
		render_scene();
	} else {
		hovered.x = floor(((float)(ev->event_x - offset.x)) / cellsize);
		hovered.y = floor(((float)(ev->event_y - offset.y)) / cellsize);

		if (!running)
			render_scene();
	}
}

static void
h_mapping_notify(xcb_mapping_notify_event_t *ev)
{
	if (ev->count > 0)
		xcb_refresh_keyboard_mapping(ksyms, ev);
}

static void
dispatch_event(xcb_generic_event_t *ev)
{
	switch (ev->response_type & ~0x80) {
	case XCB_CLIENT_MESSAGE:   h_client_message((void *)(ev)); break;
	case XCB_EXPOSE:           h_expose((void *)(ev)); break;
	case XCB_KEY_PRESS:        h_key_press((void *)(ev)); break;
	case XCB_KEY_RELEASE:      h_key_release((void *)(ev)); break;
	case XCB_BUTTON_PRESS:     h_button_press((void *)(ev)); break;
	case XCB_BUTTON_RELEASE:   h_button_release((void *)(ev)); break;
	case XCB_MOTION_NOTIFY:    h_motion_notify((void *)(ev)); break;
	case XCB_MAPPING_NOTIFY:   h_mapping_notify((void *)(ev)); break;
	case XCB_CONFIGURE_NOTIFY: h_configure_notify((void *)(ev)); break;
	}
}

int
main(int argc, char **argv)
{
	xcb_generic_event_t *ev;
	const char *loadpath;

	loadpath = NULL;
	gps = generations_per_second;

	while (++argv, --argc > 0) {
		if ((*argv)[0] == '-' && (*argv)[1] != '\0' && (*argv)[2] == '\0') {
			switch ((*argv)[1]) {
			case 'h': usage(); break;
			case 'v': version(); break;
			default: die("invalid option %s", *argv); break;
			}
		} else {
			if (loadpath != NULL)
				die("unexpected argument: %s", *argv);
			loadpath = *argv;
		}
	}

	create_window();

	if (NULL == loadpath) create_board(default_columns, default_rows);
	else load_board(loadpath);

	while (!should_quit) {
		if (running) {
			blockstart();

			while ((ev = xcb_poll_for_event(conn))) {
				dispatch_event(ev);
				free(ev);
			}

			if (should_quit)
				break;

			advance_to_next_generation();
			render_scene();
			blockwait(NANOSECONDS_PER_SECOND / gps);
		} else {
			/* don't burn the cpu while paused, block
			 * until the next event arrives */
			if (NULL == (ev = xcb_wait_for_event(conn)))
				die("connection to the X server was lost");

			dispatch_event(ev);
			free(ev);
		}
	}

	destroy_window();
	destroy_board();

	return 0;
}
