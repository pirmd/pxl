PXL_DIR ?= .

PXL_CFLAGS = -I$(PXL_DIR)
PXL_LDLIBS = -L$(PXL_DIR) -lpxl 

PXL_BACKEND ?= x11
PXL_BACKEND_SRC = $(PXL_DIR)/backend_$(PXL_BACKEND).c

X11_CFLAGS ?= -I/usr/X11R6/include
X11_LIBS   ?= -L/usr/X11R6/lib -lX11 -lXext

SDL_CFLAGS ?= -I/usr/local/include/SDL2
SDL_LIBS   ?= -L/usr/local/lib -lSDL2

.if $(PXL_BACKEND) == "x11"
PXL_CFLAGS += $(X11_CFLAGS)
PXL_LDLIBS += $(X11_LIBS)

.elif $(PXL_BACKEND) == "sdl"
PXL_CFLAGS += $(SDL_CFLAGS)
PXL_LDLIBS += $(SDL_LIBS)
.endif

PXL_BUILD_MODE ?= debug
.if $(PXL_BUILD_MODE) == "debug"
.elif $(PXL_BUILD_MODE) == "release"
.endif
