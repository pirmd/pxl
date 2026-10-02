# pxl - Examples

DEMO = demo_pong demo_fontview

PXL_DIR=..

-include $(PXL_DIR)/config.mk
CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra

include $(PXL_DIR)/Makefile.inc

CFLAGS  += $(PXL_CFLAGS)
CFLAGS  += -I.

LDFLAGS += $(PXL_LDLIBS)

all: $(DEMO)

.for d in $(DEMO)
$(d): $(d).c $(PXL_DIR)/libpxl.a
	$(CC) $(CFLAGS) $(d).c $(PXL_DIR)/libpxl.a -o $@ $(LDFLAGS)
.endfor

${DEMO}: $(PXL_HDR)

clean:
	rm -f $(DEMO)

.PHONY: all clean
