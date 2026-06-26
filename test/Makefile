# pxl - Unit tests

TESTS = test_pixbuf test_geom test_canvas test_draw2d

PXL_SRC  = ../canvas.c ../draw2d.c
PXL_SRC += ../canvas.h ../draw2d.h ../pixbuf.h ../geom.h ../err.h

-include "../config.mk"

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS += -I.. -I. -Istest

LDFLAGS += -L.. -lpxl
LDFLAGS += -Lstest -lstest

all: test

test: $(TESTS)
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

clean:
	rm -f $(TESTS)
	$(MAKE) -C stest clean

.PHONY: all test clean
