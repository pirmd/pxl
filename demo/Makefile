# pxl - Examples

DEMO = demo_hello_world demo_pong demo_gas demo_typewriter demo_fountain demo_fontview

PXL_DIR=..

-include $(PXL_DIR)/config.mk
CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra

X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

include $(PXL_DIR)/Makefile.inc

CFLAGS  += $(PXL_CFLAGS)
CLFAGS  += -I.

LDFLAGS += $(PXL_LDLIBS)
LDFLAGS += -lm


all: $(DEMO)

.for d in $(DEMO)
$(d): $(d).c $(PXL_DIR)/libpxl.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(d).c
.endfor

clean:
	rm -f $(DEMO)

.PHONY: all clean
