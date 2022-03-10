#include <time.h>
#include "blocksleep.h"

typedef struct timespec timespec_t;

static timespec_t begin_ts;

extern void
blocksleep_begin() {
	clock_gettime(CLOCK_MONOTONIC, &begin_ts);
}

extern void
blocksleep_end(int nanoseconds) {
	timespec_t now_ts, delta_ts, end_ts;

	end_ts = begin_ts;
	end_ts.tv_nsec += nanoseconds;

	if ((end_ts.tv_nsec / NANOSECONDS_PER_SECOND) > 0) {
		end_ts.tv_sec += end_ts.tv_nsec / NANOSECONDS_PER_SECOND;
		end_ts.tv_nsec = end_ts.tv_nsec % NANOSECONDS_PER_SECOND;
	}

	clock_gettime(CLOCK_MONOTONIC, &now_ts);

	/* check if we already reached the end */
	if (now_ts.tv_sec > end_ts.tv_sec) return;

	if (now_ts.tv_sec == end_ts.tv_sec &&
		now_ts.tv_nsec >= end_ts.tv_nsec) return;

	delta_ts.tv_sec = end_ts.tv_sec - now_ts.tv_sec;
	delta_ts.tv_nsec = end_ts.tv_nsec - now_ts.tv_nsec;

	if (delta_ts.tv_nsec < 0) {
		delta_ts.tv_nsec += NANOSECONDS_PER_SECOND;
		--delta_ts.tv_sec;
	}

	nanosleep(&delta_ts, 0);
}
