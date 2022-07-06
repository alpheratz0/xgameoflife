/*
	Copyright (C) 2022 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place, Suite 330, Boston, MA 02111-1307 USA

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
#include <xcb/xproto.h>
#include <xcb/xcb_cursor.h>

#define UNUSED                             __attribute__((unused))

#define DEAD_COLOR                         (0x000000)
#define ALIVE_COLOR                        (0x30a3f4)
#define BORDER_COLOR                       (0x191919)
#define BAR_COLOR                          (0xf0f72a)
#define TEXT_COLOR                         (0x000000)

#define GENERATIONS_PER_SECOND             (12)
#define NANOSECONDS_PER_SECOND             (1000*1000*1000)
#define FONT_HEIGHT                        (8)
#define INFO_BAR_HEIGHT                    (20)

#define DEFAULT_COLUMNS                    (300)
#define DEFAULT_ROWS                       (300)
#define DEFAULT_CELLSIZE                   (20)

#define MIN_CELLSIZE                       (5)
#define MAX_CELLSIZE                       (50)

#define MOUSE_LEFT                         (1)
#define MOUSE_MIDDLE                       (2)
#define MOUSE_RIGHT                        (3)
#define MOUSE_WHEEL_UP                     (4)
#define MOUSE_WHEEL_DOWN                   (5)

#define KEY_SPACE                          (65)
#define KEY_N                              (57)
#define KEY_S                              (39)

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
static xcb_cursor_context_t *cctx;
static xcb_cursor_t cursors[CURSOR_COUNT];
static xcb_gcontext_t graphics[GC_COUNT];

/* board */
static int32_t rows;
static int32_t columns;
static int32_t cellsize;
static uint8_t *cells[2];

/* game status */
static int running;
static xcb_point_t hovered;

/* dragging */
static int dragging;
static xcb_point_t offset;
static xcb_point_t mousepos;

/* blocksleep */
static struct timespec begin_ts;

static void
die(const char *err)
{
	fprintf(stderr, "xgameoflife: %s\n", err);
	exit(1);
}

static void
dief(const char *err, ...)
{
	va_list list;
	fputs("xgameoflife: ", stderr);
	va_start(list, err);
	vfprintf(stderr, err, list);
	va_end(list);
	fputc('\n', stderr);
	exit(1);
}

static void *
xcalloc(size_t n, size_t size)
{
	void *p;

	if (NULL == (p = calloc(n, size))) {
		die("error while calling calloc, no memory available");
	}

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
	if (now_ts.tv_sec > end_ts.tv_sec) {
		return;
	}

	if (now_ts.tv_sec == end_ts.tv_sec && now_ts.tv_nsec >= end_ts.tv_nsec) {
		return;
	}

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

	error = NULL;
	cookie = xcb_intern_atom(conn, 1, strlen(name), name);
	reply = xcb_intern_atom_reply(conn, cookie, &error);

	if (NULL != error) {
		dief("xcb_intern_atom failed with error code: %d",
				(int)(error->error_code));
	}

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
	mask |= XCB_GC_FOREGROUND; values[1] = 0;

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

	if (xcb_request_check(conn, cookie)) {
		dief("font not found: %s", name);
	}

	mask |= XCB_GC_FOREGROUND; values[0] = foreground;
	mask |= XCB_GC_BACKGROUND; values[1] = background;
	mask |= XCB_GC_FONT;       values[2] = font;

	xcb_create_gc(conn, id, window, mask, values);
	xcb_close_font(conn, font);

	return id;
}

static void
xsize(int16_t *width, int16_t *height)
{
	xcb_generic_error_t *error;
	xcb_get_geometry_cookie_t cookie;
	xcb_get_geometry_reply_t *reply;

	error = NULL;
	cookie = xcb_get_geometry(conn, window);
	reply = xcb_get_geometry_reply(conn, cookie, &error);

	if (NULL != error) {
		dief("xcb_get_geometry failed with error code: %d",
				(int)(error->error_code));
	}

	*width = reply->width;
	*height = reply->height;

	free(reply);
}

static void
create_window(void)
{
	xcb_screen_t *screen;

	if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL))) {
		die("can't open display");
	}

	if (NULL == (screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data)) {
		xcb_disconnect(conn);
		die("can't get default screen");
	}

	if (xcb_cursor_context_new(conn, screen, &cctx) != 0) {
		die("can't create cursor context");
	}

	window = xcb_generate_id(conn);

	xcb_create_window(
		conn, XCB_COPY_FROM_PARENT, window, screen->root,
		0, 0, 800, 600, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(const uint32_t[]) {
			DEAD_COLOR,
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE |
			XCB_EVENT_MASK_POINTER_MOTION
		}
	);

	/* set WM_NAME */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		sizeof("xgameoflife") - 1, "xgameoflife"
	);

	/* set WM_CLASS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
		sizeof("xgameoflife") * 2, "xgameoflife\0xgameoflife\0"
	);

	/* add WM_DELETE_WINDOW to WM_PROTOCOLS */
	xcb_change_property(
		conn, XCB_PROP_MODE_REPLACE, window,
		xatom("WM_PROTOCOLS"), XCB_ATOM_ATOM, 32, 1,
		(const xcb_atom_t[]) { xatom("WM_DELETE_WINDOW") }
	);

	/* load graphics */
	graphics[GC_DEAD] = xcolor(DEAD_COLOR);
	graphics[GC_ALIVE] = xcolor(ALIVE_COLOR);
	graphics[GC_BORDER] = xcolor(BORDER_COLOR);
	graphics[GC_TEXT] = xfont("fixed", TEXT_COLOR, BAR_COLOR);
	graphics[GC_BAR] = xcolor(BAR_COLOR);

	/* load cursors */
	cursors[CURSOR_FLEUR] = xcb_cursor_load_cursor(cctx, "fleur");
	cursors[CURSOR_LEFT_PTR] = xcb_cursor_load_cursor(cctx, "left_ptr");

	xcb_map_window(conn, window);
	xcb_flush(conn);
}

static void
destroy_window(void)
{
	size_t i;

	for (i = 0; i < GC_COUNT; ++i) {
		xcb_free_gc(conn, graphics[i]);
	}

	for (i = 0; i < CURSOR_COUNT; ++i) {
		xcb_free_cursor(conn, cursors[i]);
	}

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
	cellsize = DEFAULT_CELLSIZE;
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

static int
count_neighbours_alive(int x, int y)
{
	int dx, dy;
	int count;

	count = -get_cell(x, y);

	for (dx = -1; dx < 2; ++dx) {
		for (dy = -1; dy < 2; ++dy) {
			count += get_cell(x + dx, y + dy) ? 1 : 0;
		}
	}

	return count;
}

static void
advance_to_next_generation(void)
{
	int x, y, n;
	uint8_t cell, *tmp;

	for (x = 0; x < columns; ++x) {
		for (y = 0; y < rows; ++y) {
			cell = get_cell(x, y);
			n = count_neighbours_alive(x, y);

			cells[1][y*columns+x] = n == 3 || (cell && n == 2);
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

	if (NULL == (fp = fopen(filename, "w"))) {
		dief("failed to open file %s: %s", filename, strerror(errno));
	}

	fprintf(fp, "%dx%d\n", columns, rows);

	for (x = 0; x < columns; ++x) {
		for (y = 0; y < rows; ++y) {
			if (get_cell(x, y)) {
				fprintf(fp, "%d,%d\n", x, y);
			}
		}
	}

	fclose(fp);
}

static void
load_board(const char *path)
{
	int x, y;
	FILE *fp;

	if (NULL == (fp = fopen(path, "r"))) {
		dief("failed to open file %s: %s", path, strerror(errno));
	}

	if (fscanf(fp, "%dx%d\n", &columns, &rows) != 2) {
		columns = DEFAULT_COLUMNS;
		rows = DEFAULT_ROWS;

		rewind(fp);
	}

	create_board(columns, rows);

	while (fscanf(fp, "%d,%d\n", &x, &y) == 2) {
		set_cell(x, y, 1);
	}

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
	int16_t width, height;

	int32_t rectc;
	static xcb_rectangle_t rects[8192];

	int32_t i;
	xcb_point_t line[2];

	xcb_rectangle_t box;
	char text[200] = "* RUNNING";

	rectc = 0;

	xsize(&width, &height);
	xcb_clear_area(conn, 0, window, 0, 0, width, height);

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
						conn, window, graphics[GC_ALIVE],
						rectc, rects
					);

					rectc = 0;
				}
			}
		}
	}

	if (rectc != 0) {
		xcb_poly_fill_rectangle(
			conn, window, graphics[GC_ALIVE],
			rectc, rects
		);
	}

#define DRAW_LINE(x0,y0,x1,y1) do {                             \
	line[0].x = (x0); line[0].y = (y0);                         \
	line[1].x = (x1); line[1].y = (y1);                         \
	xcb_poly_line(                                              \
		conn, XCB_COORD_MODE_ORIGIN, window,                    \
		graphics[GC_BORDER], 2, line                            \
	);                                                          \
} while (0)

	for (i = -1; i < vcolumns + 1; ++i) {
		DRAW_LINE(i*cellsize+celloffx, 0, i*cellsize+celloffx, height);
	}

	for (i = -1; i < vrows + 1; ++i) {
		DRAW_LINE(0, i*cellsize+celloffy, width, i*cellsize+celloffy);
	}

#undef DRAW_LINE

	if (!running) {
		snprintf(text, sizeof(text), "* PAUSED (%hd, %hd)", hovered.x, hovered.y);
	}

	box.x = 0;
	box.y = height - INFO_BAR_HEIGHT;
	box.width = width;
	box.height = INFO_BAR_HEIGHT;

	xcb_poly_fill_rectangle(
		conn, window, graphics[GC_BAR], 1, &box
	);

	xcb_image_text_8_checked(
		conn, strlen(text), window, graphics[GC_TEXT], INFO_BAR_HEIGHT / 2,
		height - (INFO_BAR_HEIGHT - FONT_HEIGHT) / 2, text
	);

	xcb_flush(conn);
}

static int
match_opt(const char *in, const char *sh, const char *lo)
{
	return (strcmp(in, sh) == 0) || (strcmp(in, lo) == 0);
}

static inline void
print_opt(const char *sh, const char *lo, const char *desc)
{
	printf("%7s | %-25s %s\n", sh, lo, desc);
}

static void
usage(void)
{
	puts("Usage: xgameoflife [ -hv ] [ -l FILE ]");
	puts("Options are:");
	print_opt("-h", "--help", "display this message and exit");
	print_opt("-v", "--version", "display the program version");
	print_opt("-l", "--load", "load saved board");
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
	if (ev->data.data32[0] == xatom("WM_DELETE_WINDOW")) {
		destroy_window();
		destroy_board();
		exit(0);
	}
}

static void
h_expose(UNUSED xcb_expose_event_t *ev)
{
	render_scene();
}

static void
h_key_press(xcb_key_press_event_t *ev)
{
	switch (ev->detail) {
		case KEY_SPACE:
			if (!dragging) {
				running = !running;
				render_scene();
			}
			break;
		case KEY_N:
			if (!running) {
				advance_to_next_generation();
				render_scene();
			}
			break;
		case KEY_S:
			if (!running && ev->state & XCB_MOD_MASK_CONTROL) {
				save_board();
			}
			break;
	}
}

static void
h_button_press(xcb_button_press_event_t *ev)
{
	int16_t width, height;
	int16_t zoom;

	if (running) {
		return;
	}

	zoom = 0;

	switch (ev->detail) {
		case MOUSE_LEFT:
			toggle_cell(hovered.x, hovered.y);
			render_scene();
			break;
		case MOUSE_MIDDLE:
			dragging = 1;
			mousepos.x = ev->event_x;
			mousepos.y = ev->event_y;
			xcb_change_window_attributes(conn, window, XCB_CW_CURSOR, &cursors[CURSOR_FLEUR]);
			xcb_flush(conn);
			break;
		case MOUSE_WHEEL_UP:
			if (cellsize < MAX_CELLSIZE) {
				zoom = 1;
			}
			break;
		case MOUSE_WHEEL_DOWN:
			if (cellsize > MIN_CELLSIZE) {
				zoom = -1;
			}
			break;
	}

	if (zoom) {
		xsize(&width, &height);
		offset.x = (offset.x * (cellsize + zoom) - zoom * (width / 2)) / cellsize;
		offset.y = (offset.y * (cellsize + zoom) - zoom * (height / 2)) / cellsize;
		cellsize += zoom;
		render_scene();
	}
}

static void
h_button_release(xcb_button_release_event_t *ev)
{
	if (ev->detail == MOUSE_MIDDLE) {
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
	}
	else {
		hovered.x = floor(((float)(ev->event_x - offset.x)) / cellsize);
		hovered.y = floor(((float)(ev->event_y - offset.y)) / cellsize);

		if (!running) {
			render_scene();
		}
	}
}

int
main(int argc, char **argv)
{
	xcb_generic_event_t *ev;
	const char *loadpath = NULL;

	if (++argv, --argc > 0) {
		if (match_opt(*argv, "-l", "--load") && --argc > 0) loadpath = *++argv;
		else if (match_opt(*argv, "-h", "--help")) usage();
		else if (match_opt(*argv, "-v", "--version")) version();
		else if (**argv == '-') dief("invalid option %s", *argv);
		else dief("unexpected argument: %s", *argv);
	}

	create_window();

	if (NULL == loadpath) create_board(DEFAULT_COLUMNS, DEFAULT_ROWS);
	else load_board(loadpath);

	while (1) {
		blockstart();
		while ((ev = xcb_poll_for_event(conn))) {
			switch (ev->response_type & ~0x80) {
				case XCB_CLIENT_MESSAGE:
					h_client_message((xcb_client_message_event_t *)(ev));
					break;
				case XCB_EXPOSE:
					h_expose((xcb_expose_event_t *)(ev));
					break;
				case XCB_KEY_PRESS:
					h_key_press((xcb_key_press_event_t *)(ev));
					break;
				case XCB_BUTTON_PRESS:
					h_button_press((xcb_button_press_event_t *)(ev));
					break;
				case XCB_BUTTON_RELEASE:
					h_button_release((xcb_button_release_event_t *)(ev));
					break;
				case XCB_MOTION_NOTIFY:
					h_motion_notify((xcb_motion_notify_event_t *)(ev));
					break;
				default:
					break;
			}

			free(ev);
		}

		if (running) {
			advance_to_next_generation();
			render_scene();
			blockwait(NANOSECONDS_PER_SECOND / GENERATIONS_PER_SECOND);
		}
	}

	return 0;
}
