#ifndef __XGAMEOFLIFE_LF_SLEEP_H__
#define __XGAMEOFLIFE_LF_SLEEP_H__

#define NANOSECONDS_IN_ONE_SECOND (1000*1000*1000)

void lfsleep_begin();
void lfsleep_end(int nanoseconds);

#endif
