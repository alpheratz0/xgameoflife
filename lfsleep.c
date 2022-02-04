#include <time.h>
#include "lfsleep.h"

static struct timespec begin_timespec;

extern void
lfsleep_begin() {
    clock_gettime(CLOCK_MONOTONIC, &begin_timespec);
}

extern void
lfsleep_end(int nanoseconds) {
	struct timespec now_timespec;
	struct timespec delta_timespec;
	struct timespec end_timespec = begin_timespec;

	end_timespec.tv_nsec += nanoseconds;
	if ((end_timespec.tv_nsec / NANOSECONDS_IN_ONE_SECOND) > 0) {
		end_timespec.tv_sec += end_timespec.tv_nsec / NANOSECONDS_IN_ONE_SECOND;
		end_timespec.tv_nsec = end_timespec.tv_nsec % NANOSECONDS_IN_ONE_SECOND;
	}

	clock_gettime(CLOCK_MONOTONIC, &now_timespec);

	/* check if we already reached the end */
	if (now_timespec.tv_sec > end_timespec.tv_sec)
		return;

	if (now_timespec.tv_sec == end_timespec.tv_sec && now_timespec.tv_nsec >= end_timespec.tv_nsec)
		return;
	
	delta_timespec.tv_sec = 0;
	delta_timespec.tv_nsec = end_timespec.tv_nsec - now_timespec.tv_nsec;

	if (delta_timespec.tv_nsec < 0)
		delta_timespec.tv_nsec += NANOSECONDS_IN_ONE_SECOND;

	nanosleep(&delta_timespec, 0);
}
