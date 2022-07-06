/*
	Copyright (C) 2022 <alpheratz99@protonmail.com>

	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU General Public License version 2 as published by the
	Free Software Foundation.

	This program is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_COLUMNS                    (300)
#define DEFAULT_ROWS                       (300)
#define DEFAULT_ALIVE_PROB                 (0.3)

static inline float
frand(void)
{
	return ((float)(rand())) / RAND_MAX;
}

int
main(int argc, char **argv)
{
	int x, y, r, c;
	float p;

	r = DEFAULT_ROWS;
	c = DEFAULT_COLUMNS;
	p = DEFAULT_ALIVE_PROB;

	for (--argc, ++argv; argc > 0; --argc, ++argv) {
		if (strcmp(*argv, "-r") == 0 && (argc - 1) > 0) --argc, r = atoi(*++argv);
		else if (strcmp(*argv, "-c") == 0 && (argc - 1) > 0) --argc, c = atoi(*++argv);
		else if (strcmp(*argv, "-p") == 0 && (argc - 1) > 0) --argc, p = atof(*++argv);
	}

	p = p < 0 ? 0 : p > 1 ? 1 : p;

	srand((unsigned)(getpid()));
	printf("%dx%d\n", c, r);

	if (p != 0) {
		for (y = 0; y < r; ++y) {
			for (x = 0; x < c; ++x) {
				if (frand() < p) {
					printf("%d,%d\n", x, y);
				}
			}
		}
	}

	return 0;
}
