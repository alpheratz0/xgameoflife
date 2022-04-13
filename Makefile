VERSION = 0.1.1
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man
LDLIBS = -lxcb -lm
LDFLAGS = -s ${LDLIBS}
INCS = -I. -I/usr/include
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os ${INCS} -D_XOPEN_SOURCE=500 -DVERSION=\"${VERSION}\"
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

install: all
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f xgameoflife ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/xgameoflife
	@mkdir -p ${DESTDIR}${MANPREFIX}/man6
	@cp -f man/xgameoflife.6 ${DESTDIR}${MANPREFIX}/man6
	@chmod 644 ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6

dist: clean
	@mkdir -p xgameoflife-${VERSION}
	@cp -R LICENSE Makefile README man src xgameoflife-${VERSION}
	@tar -cf xgameoflife-${VERSION}.tar xgameoflife-${VERSION}
	@gzip xgameoflife-${VERSION}.tar
	@rm -rf xgameoflife-${VERSION}

uninstall:
	@rm -f ${DESTDIR}${PREFIX}/bin/xgameoflife
	@rm -f ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6

clean:
	@rm -f xgameoflife xgameoflife-${VERSION}.tar.gz ${OBJ}

.PHONY: all clean install uninstall dist
