# pxl - Unit tests

TESTS = test_pixbuf

STEST_LIB = stest/libstest.a
PXL_LIB   = ../libpxl.a

# --- Tools & Flags --------------------------------------------------------------
CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -pedantic -Wall -Wextra -Wshadow -Wvla
CFLAGS += -I../include -I../src -Istest -I.
LDFLAGS += -L.. -lpxl -Lstest -lstest

# --- Targets ------------------------------------------------------------------
all: $(TESTS)

$(STEST_LIB):
	$(MAKE) -C stest

.for t in $(TESTS)
$(t): $(t).c $(PXL_LIB) $(STEST_LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<
.endfor

test: $(TESTS)
	@for t in $(TESTS); do \
		printf "%s" "$${t}... "; \
		./$${t} >/dev/null 2>/dev/null || { echo "fail"; exit 1; }; \
		echo "ok"; \
		done

clean:
	rm -f $(TESTS)
	$(MAKE) -C stest clean

.PHONY: all test clean
