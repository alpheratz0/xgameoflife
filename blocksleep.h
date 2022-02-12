#ifndef __XGAMEOFLIFE_BLOCKSLEEP_H__
#define __XGAMEOFLIFE_BLOCKSLEEP_H__

#define NANOSECONDS_IN_ONE_SECOND (1000*1000*1000)

extern void
blocksleep_begin(void);

extern void
blocksleep_end(int nanoseconds);

#endif
