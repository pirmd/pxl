# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

SRC = draw.c draw_extra.c ascii.c text.c tileset.c
HDR = buf.h bitmask.h geom.h err.h canvas.h draw.h draw_extra.h ascii.h stepper.h text.h tileset.h

-include "config.mk"

CC     ?= cc
CFLAGS  = -std=c99 -Wall -Wextra -I.

.if defined(RELEASE)
CFLAGS += -O2 -DNDEBUG
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
.else
invalid_backend:
	@echo "Invalid BACKEND: ${BACKEND} - must be x11 or sdl"
	@exit 1

all: invalid_backend
.endif

CFLAGS_LINT  = -Wall -Wextra -Wpedantic -Werror -fsyntax-only
CFLAGS_LINT += -Wshadow -Wvla
CFLAGS_LINT += -Wwrite-strings -Wold-style-definition
CFLAGS_LINT += -Wno-unused-function

all: $(LIB)

OBJ = ${SRC:.c=.o}

$(LIB): $(OBJ)
	ar rcs $@ $?

${OBJ}: $(HDR)

lint:
	$(CC) $(CFLAGS) $(CFLAGS_LINT) $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(CFLAGS_LINT) test/*.c
	$(CC) $(CFLAGS) $(CFLAGS_LINT) -Itest/stest test/stest/*.c test/stest/*.h test/stest/examples/*.c
	$(CC) $(CFLAGS) $(CFLAGS_LINT) demo/*.c

test: $(LIB)
	$(MAKE) -C test all
	$(MAKE) -C demo clean all BACKEND=$(BACKEND)

demo: $(LIB)
	$(MAKE) -C demo BACKEND=$(BACKEND)

clean:
	rm -f *.o $(LIB)
	$(MAKE) -C test clean
	$(MAKE) -C demo clean

.PHONY: all lint test clean demo
