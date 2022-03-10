#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "util.h"

extern void
die(const char *err) {
	fprintf(stderr, "xgameoflife: %s\n", err);
	exit(1);
}

extern void
dief(const char *err, ...) {
	va_list list;
	fputs("xgameoflife: ", stderr);
	va_start(list, err);
	vfprintf(stderr, err, list);
	va_end(list);
	fputs("\n", stderr);
	exit(1);
}
