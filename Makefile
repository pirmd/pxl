# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

SRC = src/canvas.c src/draw2d.c
HDR = src/canvas.h src/draw2d.h src/pixbuf.h src/geom.h include/err.h
OBJ = ${SRC:.c=.o}

-include "config.mk"

CC     ?= cc
CFLAGS  = -std=c99 -Wall -Wextra
CFLAGS += -Iinclude -Isrc

# SDL2 flags (BSD Make compatible)
SDL_CFLAGS = -I/usr/local/include/SDL2
SDL_LIBS   = -L/usr/local/lib -lSDL2

.if defined(RELEASE)
CFLAGS += -O2
.else
CFLAGS += -O0 -g
.endif

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $?

src/canvas.o: src/canvas.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

src/draw2d.o: src/draw2d.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(LIB)
	$(MAKE) -C test all

examples: $(LIB)
	$(MAKE) -C examples all

clean:
	rm -f $(OBJ) $(LIB)
	$(MAKE) -C test clean
	$(MAKE) -C examples clean

.PHONY: all test clean examples
