#ifndef __XGAMEOFLIFE_BOARD_H__
#define __XGAMEOFLIFE_BOARD_H__

#define ROWS 300
#define COLUMNS 300

struct board {
	bool cells[ROWS*COLUMNS];
};

extern bool 
board_get(struct board *board, int x, int y);

extern void
board_set(struct board *board, int x, int y, bool value);

extern void
board_toggle(struct board *board, int x, int y);

extern void
board_save(struct board *board);

extern void
board_load(struct board *board, const char *path);

#endif
