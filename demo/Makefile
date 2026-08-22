# pxl - Examples

DEMO = demo_pong demo_gas demo_typewriter demo_fountain

-include "../config.mk"

CC      ?= cc
CFLAGS  ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS  += -I.. -I.

LDFLAGS += -L.. -lpxl -lm

X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

BACKEND ?= x11
.if ${BACKEND} == "x11"
LDFLAGS += $(X11_LIBS)
.elif ${BACKEND} == "sdl"
LDFLAGS += $(SDL_LIBS)
.endif

all: $(DEMO)

.for d in $(DEMO)
$(d): $(d).c ../libpxl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(d).c
.endfor

clean:
	rm -f $(DEMO)

.PHONY: all clean
