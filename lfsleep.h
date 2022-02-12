#ifndef __XGAMEOFLIFE_LF_SLEEP_H__
#define __XGAMEOFLIFE_LF_SLEEP_H__

#define NANOSECONDS_IN_ONE_SECOND (1000*1000*1000)

extern void
lfsleep_begin(void);

extern void
lfsleep_end(int nanoseconds);

#endif
