VERSION = 0.1.0
PREFIX = /usr/local
LDLIBS = -lxcb -lm
LDFLAGS = -s ${LDLIBS}
INCS = -I. -I/usr/include
CFLAGS = -pedantic -Wall -Wextra -Os ${INCS} -DVERSION=\"${VERSION}\"
CC = cc

SRC = src/xgameoflife.c \
	  src/blocksleep.c \
	  src/board.c \
	  src/debug.c

OBJ = ${SRC:.c=.o}

all: xgameoflife

${OBJ}: src/board.h \
		src/config.h \
		src/input.h \
		src/blocksleep.h \
		src/debug.h

xgameoflife: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f xgameoflife ${OBJ}

install: all
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f xgameoflife ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/xgameoflife

uninstall:
	@rm -f ${DESTDIR}${PREFIX}/bin/xgameoflife

.PHONY: all clean install uninstall
