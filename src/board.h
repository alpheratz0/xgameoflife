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

#ifndef __XGAMEOFLIFE_BOARD_H__
#define __XGAMEOFLIFE_BOARD_H__

#define ROWS 300
#define COLUMNS 300

struct board {
	bool cells[ROWS*COLUMNS];
};

typedef struct board board_t;

extern bool
board_get(board_t *board, int x, int y);

extern void
board_set(board_t *board, int x, int y, bool value);

extern void
board_toggle(board_t *board, int x, int y);

extern void
board_save(board_t *board);

extern void
board_load(board_t *board, const char *path);

#endif
