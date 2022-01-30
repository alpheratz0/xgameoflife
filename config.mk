PREFIX = /usr/local
INCS = -I. -I/usr/include
LIBS = -lxcb -lm
CFLAGS = -pedantic -Wall -Wextra -Os ${INCS}
LDFLAGS = -s ${LIBS}
CC = cc
