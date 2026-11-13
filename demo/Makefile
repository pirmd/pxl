# pxl - Examples

DEMO = demo_pong demo_fontview

PXL_DIR=..

-include $(PXL_DIR)/config.mk
CC     ?= cc
CFLAGS ?= -O0 -g
# Mandatory flags: appended to whatever base came from sys.mk, the
# environment or config.mk
CFLAGS += -std=c99 -Wall -Wextra

include $(PXL_DIR)/Makefile.inc

CFLAGS  += $(PXL_CFLAGS)
CFLAGS  += -I.

LDFLAGS += $(PXL_LDLIBS)

all: $(DEMO)

.for d in $(DEMO)
$(d): $(d).c $(PXL_DIR)/libpxl.a
	$(CC) $(CFLAGS) $(d).c -o $@ $(LDFLAGS)
.endfor

${DEMO}: $(PXL_HDR)

lint:
	$(CC) -Werror -fsyntax-only $(CFLAGS) $(CFLAGS_LINT) $(DEMO:%=%.c)

clean:
	rm -f $(DEMO)

.PHONY: all lint clean
