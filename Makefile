# pxl - Minimal 2D Graphics Library

LIB = libpxl.a

SRC = err.c shape.c blit.c text_basic.c text.c tileset.c

-include config.mk
CC     ?= cc
CFLAGS ?= -O0 -g
# Mandatory flags: appended to whatever base came from sys.mk, the
# environment or config.mk
CFLAGS += -std=c99 -Wall -Wextra

include Makefile.inc
SRC    += $(PXL_BACKEND_SRC)
CFLAGS += $(PXL_CFLAGS)

all: $(LIB)

OBJ = ${SRC:.c=.o}

$(LIB): $(OBJ)
	ar rcs $@ $?

${OBJ}: $(PXL_HDR)

lint:
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) $(SRC) $(PXL_HDR)
	$(MAKE) -C test lint
	$(MAKE) -C demo lint
	$(MAKE) -C tool lint

test: $(LIB)
	$(MAKE) -C test test

demo: $(LIB)
	$(MAKE) -C demo

tool:
	$(MAKE) -C tool

# Vendoring snapshot: source tarball to drop into a consumer's vendor/pxl
# (see README "Vendoring"). One backend only, per PXL_BACKEND.
GIT_COMMIT != git rev-parse --short HEAD 2>/dev/null || echo dev
DIST       = pxl-$(GIT_COMMIT)-$(PXL_BACKEND)
DIST_FILES = LICENSE README Makefile Makefile.inc config.mk.example $(SRC) $(PXL_HDR)

dist:
	mkdir -p $(DIST)
	cp $(DIST_FILES) $(DIST)/
	tar -czf $(DIST).tar.gz $(DIST)
	rm -f $(DIST)/*
	rmdir $(DIST)

clean:
	rm -f *.o $(LIB) pxl-*.tar.gz
	$(MAKE) -C test clean
	$(MAKE) -C demo clean
	$(MAKE) -C tool clean

.PHONY: all lint test demo tool dist clean
