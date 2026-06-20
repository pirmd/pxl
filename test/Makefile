# pxl - Unit tests

TESTS = test_pixbuf test_geom test_canvas test_draw2d

PXL_SRC  = ../src/canvas.c ../src/draw2d.c
PXL_SRC += ../src/canvas.h ../src/draw2d.h ../src/pixbuf.h ../src/geom.h ../include/err.h

-include "../config.mk"

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS += -I../include -I../src -Istest -I.

LDFLAGS += -L.. -lpxl -Lstest -lstest


CFLAGS_LINT  = -Werror -fsyntax-only
CFLAGS_LINT += -Wpedantic -Wshadow -Wvla -Wstrict-prototypes -Wconversion -Wdouble-promotion
CFLAGS_LINT += -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion

all: test

test: lint $(TESTS)
	@for t in $(TESTS); do \
		printf "%s" "$${t}... "; \
		./$${t} >/dev/null 2>/dev/null || { echo "fail"; exit 1; }; \
		echo "ok"; \
		done

.for t in $(TESTS)
$(t): $(t).c ../libpxl.a stest/libstest.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(t).c
.endfor

stest/libstest.a:
	$(MAKE) -C stest libstest.a

lint: $(PXL_SRC) 
	$(CC) $(CFLAGS) $(CFLAGS_LINT) $(PXL_SRC)

clean:
	rm -f $(TESTS)
	$(MAKE) -C stest clean

.PHONY: all test lint clean
