PREFIX = /usr/local
INCS = -I. -I/usr/include
LIBS = -lxcb -lm
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os ${INCS}
LDFLAGS = -s ${LIBS}
CC = cc
