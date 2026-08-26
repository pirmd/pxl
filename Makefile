# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

HDR = err.h buf.h input.h backend.h canvas.h bitmask.h geom.h draw.h draw_ascii.h draw_extra.h text.h tileset.h stepper.h
SRC = draw.c draw_ascii.c draw_extra.c text.c tileset.c

-include config.mk
CC         ?= cc


include Makefile.inc

SRC    += $(PXL_BACKEND_SRC)

CFLAGS  ?= -std=c99 -Wall -Wextra
CFLAGS  += $(PXL_CFLAGS)

.if $(PXL_BUILD_MODE) == "debug"
CFLAGS += -O0 -g
.elif $(PXL_BUILD_MODE) == "release"
CFLAGS += -DNDEBUG
.endif


CFLAGS_LINT  = -Wall -Wextra
CFLAGS_LINT += -Wpedantic -Wshadow -Wvla
CFLAGS_LINT += -Wwrite-strings -Wold-style-definition
CFLAGS_LINT += -Wno-unused-function


all: $(LIB)

OBJ = ${SRC:.c=.o}

$(LIB): $(OBJ)
	ar rcs $@ $?

${OBJ}: $(HDR)

lint:
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) $(SRC) $(HDR)
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) test/*.c test/*.h
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) demo/*.c

test: $(LIB)
	$(MAKE) -C test test
	$(MAKE) -C demo clean all

demo: $(LIB)
	$(MAKE) -C demo

clean:
	rm -f *.o $(LIB)
	$(MAKE) -C test clean
	$(MAKE) -C demo clean

.PHONY: all lint test clean demo
