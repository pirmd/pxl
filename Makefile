# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

SRC = err.c shape.c blit.c text_basic.c text.c tileset.c

-include config.mk
CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra

include Makefile.inc
SRC    += $(PXL_BACKEND_SRC)
CFLAGS += $(PXL_CFLAGS)

CFLAGS_LINT  = -Wall -Wextra
CFLAGS_LINT += -Wpedantic -Wshadow -Wvla
CFLAGS_LINT += -Wwrite-strings -Wold-style-definition
CFLAGS_LINT += -Wno-unused-function

all: $(LIB)

OBJ = ${SRC:.c=.o}

$(LIB): $(OBJ)
	ar rcs $@ $?

${OBJ}: $(PXL_HDR)

lint:
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) $(SRC) $(HDR)
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) test/*.c test/*.h
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) demo/*.c
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) tool/*.c

test: $(LIB)
	$(MAKE) -C test test
	$(MAKE) -C demo all

demo: $(LIB)
	$(MAKE) -C demo

tool:
	$(MAKE) -C tool

clean:
	rm -f *.o $(LIB)
	$(MAKE) -C test clean
	$(MAKE) -C demo clean
	$(MAKE) -C tool clean

.PHONY: all lint test demo tool clean
