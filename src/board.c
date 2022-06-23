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

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "board.h"
#include "debug.h"

extern bool
board_get(board_t *board, int x, int y)
{
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	return board->cells[y * COLUMNS + x];
}

extern void
board_set(board_t *board, int x, int y, bool value)
{
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	board->cells[y * COLUMNS + x] = value;
}

extern void
board_toggle(board_t *board, int x, int y)
{
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	board->cells[y * COLUMNS + x] ^= true;
}

extern void
board_save(board_t *board)
{
	time_t timer;
	struct tm *tm_info;
	char filename[19];
	FILE *file;

	timer = time(NULL);
	tm_info = localtime(&timer);

	strftime(filename, sizeof(filename), "%Y%m%d%H%M%S.xg", tm_info);

	if (!(file = fopen(filename, "w"))) {
		dief("failed to open file %s: %s", filename, strerror(errno));
	}

	for (int x = 0; x < COLUMNS; ++x) {
		for (int y = 0; y < ROWS; ++y) {
			if (board_get(board, x, y)) {
				fprintf(file, "%d,%d\n", x, y);
			}
		}
	}

	fclose(file);
}

extern void
board_load(board_t *board, const char *path)
{
	int x, y;
	FILE *file;

	if (!(file = fopen(path, "r"))) {
		dief("failed to open file %s: %s", path, strerror(errno));
	}

	while (fscanf(file, "%d,%d\n", &x, &y) == 2) {
		board_set(board, x, y, true);
	}

	fclose(file);
}
