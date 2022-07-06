VERSION = 0.1.0

CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Os -DVERSION=\"${VERSION}\"
LDLIBS  = -lxcb -lxcb-cursor -lm
LDFLAGS = -s ${LDLIBS}

PREFIX    = /usr/local
MANPREFIX = ${PREFIX}/share/man

all: xgameoflife xggen

.c.o:
	${CC} -c ${CFLAGS} $<

xgameoflife: xgameoflife.o
	${CC} -o $@ $< ${LDFLAGS}

xggen: xggen.o
	${CC} -o $@ $<

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f xgameoflife ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/xgameoflife
	cp -f xggen ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/xggen
	mkdir -p ${DESTDIR}${MANPREFIX}/man6
	cp -f xgameoflife.6 ${DESTDIR}${MANPREFIX}/man6
	chmod 644 ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	cp -f xggen.1 ${DESTDIR}${MANPREFIX}/man1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/xggen.1

dist: clean
	mkdir -p xgameoflife-${VERSION}
	cp -R LICENSE Makefile README xgameoflife.6 xgameoflife.c xggen.1 xggen.c xgameoflife-${VERSION}
	tar -cf xgameoflife-${VERSION}.tar xgameoflife-${VERSION}
	gzip xgameoflife-${VERSION}.tar
	rm -rf xgameoflife-${VERSION}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xgameoflife
	rm -f ${DESTDIR}${PREFIX}/bin/xggen
	rm -f ${DESTDIR}${MANPREFIX}/man6/xgameoflife.6
	rm -f ${DESTDIR}${MANPREFIX}/man1/xggen.1

clean:
	rm -f xgameoflife xggen xgameoflife.o xggen.o xgameoflife-${VERSION}.tar.gz

.PHONY: all clean install uninstall dist
