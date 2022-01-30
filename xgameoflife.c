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

*/

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "input.h"
#include "config.h"
#include "board.h"

#define FONT_HEIGHT 8

struct context {
	bool paused;
	int cell_size;
	int hovered_x;
	int hovered_y;
};

struct drag_info {
	bool active;
	float offset_x;
	float offset_y;
	float prev_x;
	float prev_y;
};

struct dimensions {
	int width;
	int height;
};

static struct board board = { 0 };
static struct context context = { true, 20, 0, 0 };
static struct drag_info board_drag_info = { 0 };
static struct dimensions wnd_size = { 0 };

static xcb_connection_t *connection;
static xcb_drawable_t window;

static xcb_atom_t wm_delete_window;
static xcb_atom_t wm_protocols;

static xcb_gcontext_t gc_alive;
static xcb_gcontext_t gc_dead;
static xcb_gcontext_t gc_border;
static xcb_gcontext_t gc_status_text;
static xcb_gcontext_t gc_status_bar;

static void
die(const char *err) {
	fprintf(stderr, "xgameoflife: %s\n", err);
	exit(1);
}

static void
set_wm_name(const char *title) {
	xcb_change_property(
		connection,
		XCB_PROP_MODE_REPLACE,
		window,
		XCB_ATOM_WM_NAME,
		XCB_ATOM_STRING,
		8,
		strlen(title),
		title
	);
}

static void
set_wm_class(const char *class) {
	xcb_change_property(
		connection,
		XCB_PROP_MODE_REPLACE,
		window,
		XCB_ATOM_WM_CLASS,
		XCB_ATOM_STRING,
		8,
		strlen(class),
		class
	);
}

static void
set_wm_protocols(void) {
	wm_delete_window = xcb_intern_atom_reply(
		connection,
		xcb_intern_atom_unchecked(
			connection, 1,
			sizeof("WM_DELETE_WINDOW") - 1,
			"WM_DELETE_WINDOW"
		),
		NULL
	)->atom;

	wm_protocols = xcb_intern_atom_reply(
		connection,
		xcb_intern_atom_unchecked(
			connection, 1,
			sizeof("WM_PROTOCOLS") - 1,
			"WM_PROTOCOLS"
		),
		NULL
	)->atom;

	xcb_change_property(
		connection,
		XCB_PROP_MODE_REPLACE,
		window,
		wm_protocols,
		XCB_ATOM_ATOM,
		32,
		1,
		&wm_delete_window
	);
}

static xcb_gcontext_t
create_gcontext_font(const char *name, unsigned int fg, unsigned int bg) {
	xcb_font_t font = xcb_generate_id (connection);
	xcb_gcontext_t gc_font = xcb_generate_id(connection);
	xcb_void_cookie_t font_cookie = xcb_open_font_checked(connection, font,	strlen(name), name);

	if (xcb_request_check(connection, font_cookie)) {
		xcb_disconnect(connection);
		die("can't open font");
	}

	xcb_create_gc(
		connection,
		gc_font,
		window,
		XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT,
		(const unsigned int[3]) {
			fg,
			bg,
			font
		}
	);

	xcb_close_font(connection, font);
	return gc_font;
}

static xcb_gcontext_t
create_gcontext_with_foreground(unsigned int fg_color) {
	xcb_gcontext_t gc = xcb_generate_id(connection);
	xcb_create_gc(
		connection,
		gc,
		window,
		XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES,
		(const unsigned int[2]){
			/* foreground color */
			fg_color,
			/* no graphics exposures */
			0
		}
	);
	return gc;
}

static void
draw_text(xcb_gcontext_t gc, short x, short y, const char *text) {
	xcb_image_text_8_checked(connection, strlen(text), window, gc, x, y, text);
}

static void
draw(int width, int height) {
	/* update window dimensions */
	wnd_size.width = width;
	wnd_size.height = height;

	/* render the cells */
	int cols = width / context.cell_size;
	int rows = height / context.cell_size;

	int start_x = -(board_drag_info.offset_x / context.cell_size);
	int start_y = -(board_drag_info.offset_y / context.cell_size);

	int cell_offset_x = ((int)board_drag_info.offset_x) % context.cell_size;
	int cell_offset_y = ((int)board_drag_info.offset_y) % context.cell_size;

	int alive_rects_c = 0;

	xcb_rectangle_t *alive_rects = malloc(sizeof(xcb_rectangle_t) * (rows + 2) * (cols * 2));

	xcb_clear_area(connection, 0, window, 0, 0, width, height);

	for (int x = -1; x <= cols + 1; ++x) {
		for (int y = -1; y <= rows + 1; ++y) {
			int mapped_x = start_x + x;
			int mapped_y = start_y + y;
			if (board_get(&board, mapped_x, mapped_y)) {
				alive_rects[alive_rects_c].x = cell_offset_x + x * context.cell_size;
				alive_rects[alive_rects_c].y = cell_offset_y + y * context.cell_size;
				alive_rects[alive_rects_c].width = context.cell_size;
				alive_rects[alive_rects_c].height = context.cell_size;
				alive_rects_c++;
			}
		}
	}

	xcb_poly_fill_rectangle(connection, window, gc_alive, alive_rects_c, alive_rects);
	free(alive_rects);

	/* render the grid */
	for (int i = -1; i <= cols + 1; ++i) {
		xcb_point_t points[2] = {
			{ cell_offset_x + i * context.cell_size, 0 },
			{ cell_offset_x + i * context.cell_size, height }
		};
		xcb_poly_line(connection, XCB_COORD_MODE_ORIGIN, window, gc_border, 2, points);
	}

	for (int j = -1; j <= rows + 1; ++j) {
		xcb_point_t points[2] = {
			{ 0, cell_offset_y + j * context.cell_size },
			{ width, cell_offset_y + j * context.cell_size }
		};
		xcb_poly_line(connection, XCB_COORD_MODE_ORIGIN, window, gc_border, 2, points);
	}

	/* render status bar at bottom */
	xcb_rectangle_t status_bar_rect = { 0, wnd_size.height - status_bar_height, wnd_size.width, status_bar_height };
	xcb_poly_fill_rectangle(connection, window, gc_status_bar, 1, &status_bar_rect);

	if(context.paused) {
		char hovered_cell_text[16];
		sprintf(hovered_cell_text, "(%d, %d)", context.hovered_x, context.hovered_y);
		draw_text(
			gc_status_text,
			status_bar_height / 2,
			wnd_size.height - ((status_bar_height - FONT_HEIGHT) / 2),
			hovered_cell_text
		);
	} else {
		draw_text(
			gc_status_text,
			status_bar_height / 2,
			wnd_size.height - ((status_bar_height - FONT_HEIGHT) / 2),
			"* RUNNING"
		);
	}

	xcb_flush(connection);
}

static void
redraw(void) {
	draw(wnd_size.width, wnd_size.height);
}

static void
advance_to_next_generation(void) {
	int neighbours_alive = 0;
	struct board board_copy = board;

	for (int x = 0; x < COLUMNS; ++x) {
		for (int y = 0; y < ROWS; ++y) {

			neighbours_alive = 0;
			for (int dx = -1; dx < 2; ++dx) {
				for (int dy = -1; dy < 2; ++dy) {
					if (board_get(&board_copy, x + dx, y + dy))
						++neighbours_alive;
				}
			}

			/* follow the rules */
			if (!board_get(&board_copy, x, y)) {
				if (neighbours_alive == 3) {
					board_set(&board, x, y, true);
				}
				continue;
			}

			if (neighbours_alive < 3 || neighbours_alive > 4) {
				board_set(&board, x, y, false);
			}
		}
	}
}

static void
loop(void) {
	advance_to_next_generation();
	redraw();
}

static void
mouse_down(xcb_button_press_event_t *ev) {
	switch(ev->detail) {
		case MOUSE_LEFT:
			if (context.paused && ev->event_y < (wnd_size.height - 20)) {
				board_toggle(&board, context.hovered_x, context.hovered_y);
				redraw();
			}
			break;
		case MOUSE_MIDDLE:
			if (context.paused) {
				board_drag_info.active = true;
				board_drag_info.prev_x = ev->event_x;
				board_drag_info.prev_y = ev->event_y;
			}
			break;
		case MOUSE_WHEEL_UP:
			if (context.paused && context.cell_size < max_cell_size) {
				/* zoom-in to the center */
				board_drag_info.offset_x = (board_drag_info.offset_x * (context.cell_size + 1) - (wnd_size.width / 2)) / context.cell_size;
				board_drag_info.offset_y = (board_drag_info.offset_y * (context.cell_size + 1) - (wnd_size.height / 2)) / context.cell_size;
				context.cell_size++;
				redraw();
			}
			break;
		case MOUSE_WHEEL_DOWN:
			if (context.paused && context.cell_size > min_cell_size) {
				/* zoom-out from the center */
				board_drag_info.offset_x = (board_drag_info.offset_x * (context.cell_size - 1) + (wnd_size.width / 2)) / context.cell_size;
				board_drag_info.offset_y = (board_drag_info.offset_y * (context.cell_size - 1) + (wnd_size.height / 2)) / context.cell_size;
				context.cell_size--;
				redraw();
			}
			break;
	}
}

static void
mouse_up(xcb_button_release_event_t *ev) {
	switch(ev->detail) {
		case MOUSE_MIDDLE:
			board_drag_info.active = false;
			break;
	}
}

static void
mouse_move(xcb_motion_notify_event_t *ev) {
	if (board_drag_info.active) {
		board_drag_info.offset_x += ev->event_x - board_drag_info.prev_x;
		board_drag_info.offset_y += ev->event_y - board_drag_info.prev_y;
		board_drag_info.prev_x = ev->event_x;
		board_drag_info.prev_y = ev->event_y;
		redraw();
	} else {
		/* map mouse position to grid position */
		context.hovered_x = floor((board_drag_info.offset_x * -1 + ev->event_x) / context.cell_size);
		context.hovered_y = floor((board_drag_info.offset_y * -1 + ev->event_y) / context.cell_size);

		if (context.paused) {
			redraw();
		}
	}
}

static void
key_down(xcb_key_press_event_t *ev) {
	switch(ev->detail) {
		case KEY_SPACE:
			context.paused = !context.paused;
			redraw();
			break;
		case KEY_N:
			if (context.paused) {
				advance_to_next_generation();
				redraw();
			}
			break;
		case KEY_S:
			/* save the current board */
			break;
	}
}

int
main(void) {
	/* connect to the X server using the DISPLAY env variable */
	connection = xcb_connect(NULL, NULL);

	if (xcb_connection_has_error(connection)) {
		die("can't open display");
	}

	/* get default screen */
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

	if (!screen) {
		xcb_disconnect(connection);
		die("can't get default screen");
	}

	/* setup window */
	window = xcb_generate_id(connection);

	xcb_void_cookie_t window_cookie = xcb_create_window_checked(
		connection,
		XCB_COPY_FROM_PARENT,
		window,
		screen->root,
		0, 0,
		800, 600,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
		(const unsigned int[2]) {
			/* background color */
			color_dead,
			/* event mask */
			XCB_EVENT_MASK_EXPOSURE |
			XCB_EVENT_MASK_KEY_PRESS |
			XCB_EVENT_MASK_BUTTON_PRESS |
			XCB_EVENT_MASK_BUTTON_RELEASE |
			XCB_EVENT_MASK_POINTER_MOTION
		}
	);

	if (xcb_request_check(connection, window_cookie)) {
		xcb_disconnect(connection);
		die("can't create window");
	}

	set_wm_name("conway's game of life");
	set_wm_class("xgameoflife");
	set_wm_protocols();

	/* show the window */
	xcb_void_cookie_t map_window_cookie = xcb_map_window_checked(connection, window);

	if (xcb_request_check(connection, map_window_cookie)) {
		xcb_disconnect(connection);
		die("can't map window");
	}

	xcb_flush(connection);

	/* initialize graphic contexts */
	gc_alive = create_gcontext_with_foreground(color_alive);
	gc_dead = create_gcontext_with_foreground(color_dead);
	gc_border = create_gcontext_with_foreground(color_border);
	gc_status_bar = create_gcontext_with_foreground(color_status_bar);
	gc_status_text = create_gcontext_font("fixed", color_status_text, color_status_bar);

	xcb_generic_event_t *ev;

	while (1) {
		while ((ev = xcb_poll_for_event(connection))) {
			switch (ev->response_type & ~0x80) {
				case XCB_CLIENT_MESSAGE:
					/* handle window manager request to delete the window */
					/* https://www.x.org/docs/ICCCM/icccm.pdf */
					if (((xcb_client_message_event_t *)(ev))->data.data32[0] == wm_delete_window)
						goto end;
					break;
				case XCB_EXPOSE:
					draw(((xcb_expose_event_t *)(ev))->width, ((xcb_expose_event_t *)(ev))->height);
					break;
				case XCB_KEY_PRESS:
					key_down((xcb_key_press_event_t *)(ev));
					break;
				case XCB_BUTTON_PRESS:
					mouse_down((xcb_button_press_event_t *)(ev));
					break;
				case XCB_BUTTON_RELEASE:
					mouse_up((xcb_button_release_event_t *)(ev));
					break;
				case XCB_MOTION_NOTIFY:
					mouse_move((xcb_motion_notify_event_t *)(ev));
					break;
				default:
					break;
			}
		}

		free(ev);

		if (!context.paused) {
			loop();
			nanosleep((const struct timespec[]){{0, (1000 * 1000 * 1000) / frames_per_second}}, NULL);
		}
	}

end:
	xcb_free_gc(connection, gc_alive);
	xcb_free_gc(connection, gc_dead);
	xcb_free_gc(connection, gc_border);
	xcb_free_gc(connection, gc_status_text);
	xcb_free_gc(connection, gc_status_bar);
	xcb_disconnect(connection);
	return 0;
}
