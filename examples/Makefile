# pxl - Examples

CC     ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra
CFLAGS += -I../include -I../src -I.
LDFLAGS += -L.. -lpxl

# SDL backend
SDL_CFLAGS = -I/usr/local/include/SDL2
SDL_LIBS = -L/usr/local/lib -lSDL2

demo_sdl: demo.c ../src/backend_sdl.c ../libpxl.a
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ $? $(SDL_LIBS) $(LDFLAGS)

# X11 backend
X11_CFLAGS = -I/usr/X11R6/include
X11_LIBS = -L/usr/X11R6/lib -lX11 -lXext

demo_x11: demo.c ../src/backend_x11.c ../libpxl.a
	$(CC) $(CFLAGS) $(X11_CFLAGS) -o $@ $? $(X11_LIBS) $(LDFLAGS)

# Default target
all: demo_sdl

clean:
	rm -f demo_sdl demo_x11

.PHONY: all clean
