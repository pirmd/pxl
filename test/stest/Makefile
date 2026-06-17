LIB = libstest.a
SRC = stest.c
HDR = stest.h
OBJ = ${SRC:.c=.o}

CC      ?= cc
CFLAGS  ?= -O2 -std=c99 -Wpedantic -Wall -Wextra -Wshadow -Wvla

all: ${LIB}

${OBJ}: ${SRC} ${HDR}
	$(CC) $(CFLAGS) -c ${SRC} -o $@

${LIB}: ${OBJ}
	ar rcs $@ ${OBJ}

test: ${LIB}
	${MAKE} -C examples test

clean:
	rm -f ${OBJ} ${LIB}

.PHONY: all clean test
