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
