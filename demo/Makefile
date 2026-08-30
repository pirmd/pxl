# pxl - Examples

DEMO = demo_hello_world demo_pong demo_gas demo_typewriter demo_fountain demo_fontview

PXL_DIR=..

-include $(PXL_DIR)/config.mk
CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra

include $(PXL_DIR)/Makefile.inc

CFLAGS  += $(PXL_CFLAGS)
CFLAGS  += -I.

LDFLAGS += $(PXL_LDLIBS)
# -lm required for demo_fountain.c (uses cosf/sinf from math.h)
LDFLAGS += -lm

all: $(DEMO)

.for d in $(DEMO)
$(d): $(d).c demo_helpers.o $(PXL_DIR)/libpxl.a
	$(CC) $(CFLAGS) $(d).c demo_helpers.o -o $@ $(LDFLAGS)
.endfor

demo_helpers.o: demo_helpers.c demo_helpers.h $(PXL_HDR)
	$(CC) $(CFLAGS) -c -o $@ demo_helpers.c

${DEMO}: demo_helpers.h $(PXL_HDR)

clean:
	rm -f $(DEMO) *.o

.PHONY: all clean
