#ifndef __XGAMEOFLIFE_BOARD_H__
#define __XGAMEOFLIFE_BOARD_H__

#define ROWS 300
#define COLUMNS 300

struct board {
	bool cells[ROWS*COLUMNS];
};

bool board_get(struct board *board, int x, int y);
void board_set(struct board *board, int x, int y, bool value);
void board_toggle(struct board *board, int x, int y);

#endif
