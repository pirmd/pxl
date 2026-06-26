# pxl - Examples

-include "../config.mk"

CC      ?= cc
CFLAGS  ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS  += -I.. -I.

LDFLAGS += -L.. -lpxl

X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

BACKEND ?= x11
.if ${BACKEND} == "x11"
LDFLAGS += $(X11_LIBS)
.elif ${BACKEND} == "sdl"
LDFLAGS += $(SDL_LIBS)
.endif

all: demo

demo: demo.c ../libpxl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $>

clean:
	rm -f demo

.PHONY: all clean
