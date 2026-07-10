# pxl - Unit tests

TESTS = test_buf test_geom test_canvas test_draw test_draw_extra test_stepper test_input test_backend test_ascii 

PXL_SRC  = ../draw.c ../ascii.c ../draw_extra.c
PXL_SRC += ../canvas.h ../draw.h ../ascii.h ../draw_extra.h ../buf.h ../geom.h ../err.h
PXL_SRC += ../stepper.h ../input.h ../backend.h

-include "../config.mk"

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS += -I.. -I. -Istest

LDFLAGS += -L.. -lpxl
LDFLAGS += -Lstest -lstest

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
$(t): $(t).c ../libpxl.a stest/libstest.a
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(t).c
.endfor

stest/libstest.a:
	@$(MAKE) -C stest libstest.a

clean:
	@rm -f $(TESTS)
	$(MAKE) -C stest clean

.PHONY: all test clean
