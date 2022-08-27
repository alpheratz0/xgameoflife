.POSIX:
.PHONY: all clean install uninstall dist

include config.mk

all: xgameoflife

xgameoflife: xgameoflife.o
	$(CC) $(LDFLAGS) -o xgameoflife xgameoflife.o $(LDLIBS)

clean:
	rm -f xgameoflife xgameoflife.o xgameoflife-$(VERSION).tar.gz

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f xgameoflife $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/xgameoflife
	mkdir -p $(DESTDIR)$(MANPREFIX)/man6
	cp -f xgameoflife.6 $(DESTDIR)$(MANPREFIX)/man6
	chmod 644 $(DESTDIR)$(MANPREFIX)/man6/xgameoflife.6

dist: clean
	mkdir -p xgameoflife-$(VERSION)
	cp -R COPYING config.mk Makefile README xgameoflife.6 xgameoflife.c xgameoflife-$(VERSION)
	tar -cf xgameoflife-$(VERSION).tar xgameoflife-$(VERSION)
	gzip xgameoflife-$(VERSION).tar
	rm -rf xgameoflife-$(VERSION)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/xgameoflife
	rm -f $(DESTDIR)$(MANPREFIX)/man6/xgameoflife.6
