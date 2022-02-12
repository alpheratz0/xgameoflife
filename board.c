#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "board.h"
#include "util.h"

bool
board_get(struct board *board, int x, int y) {
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	return board->cells[y * COLUMNS + x];
}

void
board_set(struct board *board, int x, int y, bool value) {
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	board->cells[y * COLUMNS + x] = value;
}

void
board_toggle(struct board *board, int x, int y) {
	x = ((x % COLUMNS) + COLUMNS) % COLUMNS;
	y = ((y % ROWS) + ROWS) % ROWS;
	board->cells[y * COLUMNS + x] ^= true;
}

void
board_save(struct board *board) {
	time_t timer = time(NULL);
	struct tm* tm_info = localtime(&timer);
	char filename[19];

	strftime(filename, sizeof(filename), "%Y%m%d%H%M%S.xg", tm_info);

	FILE *file = fopen(filename, "w");

	if (!file)
		die("couldn't create the file");

	for (int x = 0; x < COLUMNS; ++x) {
		for (int y = 0; y < ROWS; ++y) {
			if (board_get(board, x, y)) {
				fprintf(file, "%d,%d\n", x, y);
			}
		}
	}

	fclose(file);
}

void
board_load(struct board *board, const char *path) {
	int x, y;
	FILE *file = fopen(path, "r");

	if (!file)
		die("file doesn't exist");

	while (fscanf(file, "%d,%d\n", &x, &y) == 2)
		board_set(board, x, y, true);

	fclose(file);
}
