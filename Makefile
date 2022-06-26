VERSION = 1.0.0-rev+${shell git rev-parse --short=16 HEAD}
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man
LDLIBS = -lxcb -lm
LDFLAGS = -s ${LDLIBS}
INCS = -I. -I/usr/include
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os ${INCS} -DVERSION="\"${VERSION}\""
CC = cc

SRC = xgameoflife.c

OBJ = ${SRC:.c=.o}

all: xgameoflife

.c.o:
	${CC} -c ${CFLAGS} $<

xgameoflife: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f xgameoflife ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/xgameoflife
	mkdir -p ${DESTDIR}${MANPREFIX}/man6
	cp -f xgameoflife.6 ${DESTDIR}${MANPREFIX}/man6
	chmod 644 ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6

dist: clean
	mkdir -p xgameoflife-${VERSION}
	cp -R LICENSE Makefile README xgameoflife.6 xgameoflife.c xgameoflife-${VERSION}
	tar -cf xgameoflife-${VERSION}.tar xgameoflife-${VERSION}
	gzip xgameoflife-${VERSION}.tar
	rm -rf xgameoflife-${VERSION}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xgameoflife
	rm -f ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6

clean:
	rm -f xgameoflife xgameoflife-${VERSION}.tar.gz ${OBJ}

.PHONY: all clean install uninstall dist
