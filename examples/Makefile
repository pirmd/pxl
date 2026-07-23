# pxl - Examples

DEMOS = demo_sketch demo_pong demo_loops demo_gas demo_typewriter demo_fountain

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

all: $(DEMOS)

.for d in $(DEMOS)
$(d): $(d).c ../libpxl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(d).c
.endfor

clean:
	rm -f $(DEMOS)

.PHONY: all clean
