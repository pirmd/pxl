LIB = libstest.a
HDR = stest.h
OBJ = stest.o

CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -pedantic -Wall -Wextra -Wshadow -Wvla

all: ${LIB}

${OBJ}: ${HDR}

${LIB}: ${OBJ}
	ar rcs $@ ${OBJ}

test: ${LIB}
	${MAKE} -C examples test

clean:
	rm -f ${OBJ} ${LIB}

.PHONY: all clean install uninstall test
