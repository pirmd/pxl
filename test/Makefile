# pxl - Unit tests

TST  = test_buf test_input test_backend test_canvas test_geom test_draw test_draw_ascii test_draw_extra
TST += test_text test_tileset test_stepper

PXL_DIR=..

-include $(PXL_DIR)/config.mk

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra

X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

include $(PXL_DIR)/Makefile.inc

CFLAGS  += $(PXL_CFLAGS)
CFLAGS  += -I.

LDFLAGS += $(PXL_LDLIBS)

all: test

test: $(TST)
	@for t in $(TST); do \
		./$${t} >/dev/null || { echo "$${t} FAIL"; exit 1; }; \
		done

.for t in $(TST)
$(t): $(t).c test.h $(PXL_DIR)/libpxl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(t).c
.endfor


clean:
	@rm -f $(TST)

.PHONY: all test clean
