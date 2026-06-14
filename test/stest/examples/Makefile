TESTS = test_ok test_fail test_crash

CC     ?= cc
CFLAGS ?= -O2 -std=c99 -pedantic -Wall -Wextra -Wshadow -Wvla
CFLAGS += -I..
LDFLAGS += -L..
STESTDIR = ..
LIB = $(STESTDIR)/libstest.a

all: $(TESTS)

.for t in $(TESTS)
$(t): $(t).c $(LIB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIB)
.endfor

test: $(TESTS)
	@./test_ok >/dev/null 2>&1 && echo "PASS: test_ok" || (echo "FAIL: test_ok" && exit 1)
	@./test_fail >/dev/null 2>&1 && (echo "FAIL: test_fail should fail"; exit 1) || echo "PASS: test_fail"
	@./test_crash 2>&1 | grep -q "SIGABRT" && echo "PASS: crash detected" || (echo "FAIL: crash" && exit 1)

clean:
	rm -f $(TESTS)

.PHONY: all test clean
