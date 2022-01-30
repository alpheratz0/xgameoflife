#include <stdbool.h>
#include "board.h"

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
