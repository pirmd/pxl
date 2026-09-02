PXL_DIR ?= .

PXL_CFLAGS = -I$(PXL_DIR)
PXL_LDLIBS = -L$(PXL_DIR) -lpxl

PXL_BACKEND ?= x11
PXL_BACKEND_SRC = $(PXL_DIR)/backend_$(PXL_BACKEND).c

# --- Backend flags (bmake syntax, with pkg-config fallback) --------------------
.if !defined(X11_CFLAGS)
X11_CFLAGS != pkg-config --cflags x11 xext 2>/dev/null || echo "-I/usr/X11R6/include"
.endif
X11_CFLAGS += -D_DEFAULT_SOURCE

.if !defined(X11_LIBS)
X11_LIBS != pkg-config --libs x11 xext 2>/dev/null || echo "-L/usr/X11R6/lib -lX11 -lXext"
.endif

.if !defined(SDL_CFLAGS)
SDL_CFLAGS != pkg-config --cflags sdl2 2>/dev/null || echo "-I/usr/local/include/SDL2"
.endif

.if !defined(SDL_LIBS)
SDL_LIBS != pkg-config --libs sdl2 2>/dev/null || echo "-L/usr/local/lib -lSDL2"
.endif

# --- Apply backend ----------------------------------------------------------------
.if $(PXL_BACKEND) == "x11"
PXL_CFLAGS += $(X11_CFLAGS)
PXL_LDLIBS += $(X11_LIBS)
.elif $(PXL_BACKEND) == "sdl"
PXL_CFLAGS += $(SDL_CFLAGS)
PXL_LDLIBS += $(SDL_LIBS)
.endif

PXL_HDR = $(PXL_DIR)/err.h $(PXL_DIR)/buf.h $(PXL_DIR)/input.h $(PXL_DIR)/backend.h \
          $(PXL_DIR)/canvas.h $(PXL_DIR)/bitmask.h $(PXL_DIR)/geom.h $(PXL_DIR)/shape.h \
          $(PXL_DIR)/blit.h $(PXL_DIR)/text_basic.h $(PXL_DIR)/text.h $(PXL_DIR)/tileset.h \
          $(PXL_DIR)/stepper.h $(PXL_DIR)/color.h $(PXL_DIR)/app.h $(PXL_DIR)/pxl.h \
          $(PXL_DIR)/camera.h
