#include <stdio.h>
#include <stdlib.h>
#include "util.h"

extern void
die(const char *err) {
	fprintf(stderr, "xgameoflife: %s\n", err);
	exit(1);
}
