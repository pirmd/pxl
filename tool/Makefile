TOOL = bdf2pxl
TOOL_TST = test_bdf2pxl

-include ../config.mk
CC     ?= cc
CFLAGS ?= -O0 -g
# Mandatory flags: appended to whatever base came from sys.mk, the
# environment or config.mk.
# POSIX prototypes (getopt, strdup, popen, mkstemp) are requested per-file
# via _POSIX_C_SOURCE at the top of each .c, keeping flags pure -std=c99.
CFLAGS += -std=c99 -Wall -Wextra

all: $(TOOL)

.for d in $(TOOL)
$(d): $(d).c
	$(CC) $(CFLAGS) $(d).c -o $@ $(LDFLAGS)
.endfor

test: $(TOOL) $(TOOL_TST)
	@for t in $(TOOL_TST); do \
		./$${t} >/dev/null || { echo "$${t} FAIL"; exit 1; }; \
		done

.for t in $(TOOL_TST)
$(t): $(TOOL) $(t).c
	$(CC) $(CFLAGS) $(t).c -o $@ $(LDFLAGS)
.endfor

clean:
	rm -f $(TOOL) $(TOOL_TST)

.PHONY: all clean test
