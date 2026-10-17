PREFIX = /usr/local

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

PKG-CONFIG = pkg-config
CFLAGS = -Wall -Wpedantic -Wextra $(shell $(PKG-CONFIG) --cflags libpulse)
LDLIBS = $(shell $(PKG-CONFIG) --libs libpulse)

.PHONY: build clean install

volumectl: $(OBJ)

build: volumectl

clean:
	$(RM) volumectl $(OBJ)

install: build
	install -Dm0755 volumectl $(DESTDIR)$(PREFIX)/bin/volumectl
