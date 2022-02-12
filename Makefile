PREFIX = /usr/local
LDLIBS = -lxcb -lm
LDFLAGS = -s ${LDLIBS}
INCS = -I. -I/usr/include
CFLAGS = -pedantic -Wall -Wextra -Os ${INCS}
CC = cc

SRC = \
	xgameoflife.c \
	blocksleep.c \
	board.c \
	util.c

OBJ = ${SRC:.c=.o}

all: xgameoflife

${OBJ}: board.h config.h input.h blocksleep.h util.h

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
