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
		for (y = 0; y < c; ++y) {
			for (x = 0; x < r; ++x) {
				if (frand() < p) {
					printf("%d,%d\n", x, y);
				}
			}
		}
	}

	return 0;
}
