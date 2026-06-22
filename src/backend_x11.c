#include <X11/Xlib.h>      /* defines Bool, XID, etc. */
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <assert.h>

#include "backend.h"
#include "pixbuf.h"

/* Only little-endian is supported - X11 uses ARGB8888 in memory (A: bits 24-31, R: 16-23, G: 8-15, B: 0-7) */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "X11 backend only supports little-endian architectures"
#endif

#define ARGB32_DEPTH    32
#define ARGB_RED_MASK   0x00ff0000
#define ARGB_GREEN_MASK 0x0000ff00
#define ARGB_BLUE_MASK  0x000000ff

/* Global backend state */
static struct {
    Display          *display;
    Window            window;
    GC                gc;
    XShmSegmentInfo   shm;
    XImage           *img;
    int               width, height;
} g_x11;

/*  Select a 32‑bit TrueColor visual that matches ARGB masks */
static bool 
select_argb_visual(Display *display, int scr, Visual **out_visual, int *out_depth) {
    XVisualInfo vi_tmpl = {
        .class      = TrueColor,
        .depth      = ARGB32_DEPTH,
        .red_mask   = ARGB_RED_MASK,
        .green_mask = ARGB_GREEN_MASK,
        .blue_mask  = ARGB_BLUE_MASK,
    };

    int count = 0;
    XVisualInfo *vi = XGetVisualInfo(display,
        VisualClassMask | VisualDepthMask |
        VisualRedMaskMask | VisualGreenMaskMask | VisualBlueMaskMask,
        &vi_tmpl, &count);

    if (vi && count > 0) {
        *out_visual = vi->visual;
        *out_depth  = vi->depth;
        XFree(vi);
        return true;
    }

    XFree(vi);
    return false;
}

pxl_err_t
backend_init(const char *title, int w, int h, bool fullscreen) {
    (void)fullscreen;        /* TODO */

	backend_deinit();

    g_x11.display = XOpenDisplay(NULL);
    if (!g_x11.display) {
		return PXL_E_BACKEND_INIT;
	}

    int scr = DefaultScreen(g_x11.display);

    Visual *visual = NULL;
	int      depth = 0;
    if (!select_argb_visual(g_x11.display, scr, &visual, &depth)) {
		goto fail;
	}

    Window root = RootWindow(g_x11.display, scr);
    XSetWindowAttributes win_attrs = {
        .colormap = XCreateColormap(g_x11.display, root, visual, AllocNone),
    };

    g_x11.window = XCreateWindow(g_x11.display, root,
			0, 0, w, h, 0,
			depth, InputOutput, visual,
			CWColormap | CWBackPixel | CWBorderPixel,
			&win_attrs);
    if (!g_x11.window) goto fail;

    XStoreName(g_x11.display, g_x11.window, title);
    XSelectInput(g_x11.display, g_x11.window,
                 ExposureMask | KeyPressMask | KeyReleaseMask |
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

	g_x11.img = XShmCreateImage(g_x11.display, visual, depth, ZPixmap, NULL,
			&g_x11.shm, w, h);
    if (!g_x11.img) goto fail;

	/* ensure we are pixel-aligned */
	if (g_x11.img->bytes_per_line % sizeof(pix_t) != 0) {
		goto fail;
	}

    g_x11.shm.shmid = shmget(IPC_PRIVATE,
                         g_x11.img->bytes_per_line * h,
                         IPC_CREAT | 0777);
    if (g_x11.shm.shmid < 0) goto fail;

    g_x11.shm.shmaddr = g_x11.img->data = shmat(g_x11.shm.shmid, 0, 0);
    if (g_x11.shm.shmaddr == (char *)-1) goto fail;

    if (!XShmAttach(g_x11.display, &g_x11.shm)) goto fail;

    g_x11.gc = XCreateGC(g_x11.display, g_x11.window, 0, NULL);
    if (!g_x11.gc) goto fail;

    g_x11.width  = w;
    g_x11.height = h;

    XMapWindow(g_x11.display, g_x11.window);
    XFlush(g_x11.display);

    return PXL_SUCCESS;

fail:
	backend_deinit();
	return PXL_E_BACKEND_INIT;
}

void
backend_deinit(void) {
	if (!g_x11.display) return;

    if (g_x11.img) {
        XShmDetach(g_x11.display, &g_x11.shm);
        shmctl(g_x11.shm.shmid, IPC_RMID, NULL);
        XDestroyImage(g_x11.img);
        g_x11.img = NULL;
    }

    if (g_x11.gc)      XFreeGC(g_x11.display, g_x11.gc);
    if (g_x11.window)  XDestroyWindow(g_x11.display, g_x11.window);
    if (g_x11.display) XCloseDisplay(g_x11.display);

    g_x11.display = NULL;
}

pxl_err_t
backend_begin_frame(pixbuf_t *out_pb) {
	assert(g_x11.display && g_x11.img && g_x11.img->data);
	assert(g_x11.img->bytes_per_line % sizeof(pix_t) == 0);

	out_pb->width  = g_x11.width;
	out_pb->height = g_x11.height;
	out_pb->stride = g_x11.img->bytes_per_line / sizeof(pix_t);
	out_pb->data   = (pix_t *)g_x11.img->data;

	return PXL_SUCCESS;
}

void
backend_end_frame(void) {
	assert(g_x11.display && g_x11.img && g_x11.img->data);

	XShmPutImage(g_x11.display, g_x11.window, g_x11.gc,
			g_x11.img, 0, 0, 0, 0, g_x11.width, g_x11.height,
			False);

	XSync(g_x11.display, False);
}

static int
x11_keysym_to_key(KeySym keysym) {
	switch (keysym) {
		case XK_Up:     return KEY_UP;
		case XK_Down:   return KEY_DOWN;
		case XK_Left:   return KEY_LEFT;
		case XK_Right:  return KEY_RIGHT;
		case XK_space:  return KEY_SPACE;
		case XK_Escape: return KEY_ESCAPE;
		default: return -1;
	}
}

bool
backend_poll_events(input_t *input) {
	XEvent event;

	input_save_prev_state(input);

	while (XPending(g_x11.display)) {
		XNextEvent(g_x11.display, &event);

		switch (event.type) {
			/* TODO: detect quit event like SDL_Quit */

			case KeyPress:
			case KeyRelease: {
				bool pressed = (event.type == KeyPress);
				KeySym keysym = XLookupKeysym(&event.xkey, 0);
				int key = x11_keysym_to_key(keysym);
				if (key >= 0 && key < KEY_COUNT) {
					input->keys[key] = pressed;
				}
				break;
			}

			case ButtonPress:
			case ButtonRelease: {
				bool pressed = (event.type == ButtonPress);
				if (event.xbutton.button == Button1) {
					input->mouse_left = pressed;
				} else if (event.xbutton.button == Button3) {
					input->mouse_right = pressed;
				}
				break;
			}

			case MotionNotify:
				input->mouse_x = event.xmotion.x;
				input->mouse_y = event.xmotion.y;
				break;
		}
	}

	return true;
}
