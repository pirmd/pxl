# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

SRC = canvas.c draw2d.c
HDR = canvas.h draw2d.h pixbuf.h geom.h err.h

-include "config.mk"

CC     ?= cc
CFLAGS  = -std=c99 -Wall -Wextra
CFLAGS += -Iinclude -Isrc

.if defined(RELEASE)
CFLAGS += -O2
.else
CFLAGS += -O0 -g
.endif

X11_CFLAGS ?= -I/usr/X11R6/include
X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext

SDL_CFLAGS ?= -I/usr/local/include/SDL2
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

BACKEND ?= x11

.if ${BACKEND} == "x11"
SRC    += backend_x11.c
CFLAGS += $(X11_CFLAGS)
LIBS   += $(X11_LIBS)
.elif ${BACKEND} == "sdl"
SRC    += backend_sdl.c
CFLAGS += $(SDL_CFLAGS)
LIBS   += $(SDL_LIBS)
.endif


CFLAGS_LINT  = -Werror -fsyntax-only
CFLAGS_LINT += -Wpedantic -Wshadow -Wvla -Wstrict-prototypes -Wconversion -Wdouble-promotion
CFLAGS_LINT += -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion


all: $(LIB)

OBJ = ${SRC:.c=.o}

$(LIB): $(OBJ)
	ar rcs $@ $?

canvas.o: canvas.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

draw2d.o: draw2d.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

backend_x11.o: backend_x11.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

backend_sdl.o: backend_sdl.c $(HDR)
	$(CC) $(CFLAGS) -c $< -o $@

lint: $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(CFLAGS_LINT) $>

test: $(LIB)
	$(MAKE) -C test all

examples: $(LIB)
	$(MAKE) -C examples all BACKEND=$(BACKEND)

clean:
	rm -f $(OBJ) $(LIB)
	$(MAKE) -C test clean
	$(MAKE) -C examples clean

.PHONY: all lint test clean examples
