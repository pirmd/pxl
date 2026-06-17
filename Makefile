# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

CC     ?= cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra
CFLAGS += -Iinclude -Isrc

.if defined(RELEASE)
CFLAGS += -O2
.else
CFLAGS += -O0 -g
.endif

all: $(LIB)

$(LIB): src/canvas.o
	ar rcs $@ $?

src/canvas.o: src/canvas.c src/canvas.h src/pixbuf.h include/err.h
	$(CC) $(CFLAGS) -c src/canvas.c -o $@

test: $(LIB)
	$(MAKE) -C test test

clean:
	rm -f $(LIB)
	$(MAKE) -C test clean

.PHONY: all test clean
