include config.mk

SRC = xgameoflife.c lfsleep.c board.c
OBJ = ${SRC:.c=.o}

all: xgameoflife

${OBJ}: board.h config.h input.h lfsleep.h

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
