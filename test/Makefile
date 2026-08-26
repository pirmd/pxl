# pxl - Unit tests

TESTS = test_buf test_geom test_canvas test_draw test_draw_extra test_stepper test_input test_backend test_ascii test_text test_tileset

PXL_SRC  = ../draw.c ../ascii.c ../draw_extra.c ../text.c ../tileset.c
PXL_SRC += ../canvas.h ../draw.h ../ascii.h ../draw_extra.h ../buf.h ../geom.h ../err.h ../text.h ../tileset.h
PXL_SRC += ../stepper.h ../input.h ../backend.h

-include "../config.mk"

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS += -I.. -I.

LDFLAGS += -L.. -lpxl

X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

BACKEND ?= x11
.if ${BACKEND} == "x11"
LDFLAGS += $(X11_LIBS)
.elif ${BACKEND} == "sdl"
LDFLAGS += $(SDL_LIBS)
.endif

all: test

test: $(TESTS)
	@for t in $(TESTS); do \
		./$${t} >/dev/null || { echo "$${t} FAIL"; exit 1; }; \
		done

.for t in $(TESTS)
$(t): $(t).c test.h ../libpxl.a 
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(t).c
.endfor

clean:
	@rm -f $(TESTS)

.PHONY: all test clean
