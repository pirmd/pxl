#include <assert.h>
#include <limits.h>
#include <stdbool.h>  /* for bool, false, true */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>       /* for Atom, None, ClientMessage, KeySym */
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <X11/XKBlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <string.h>
#include <locale.h>

#include "backend.h"
#include "input.h"
#include "buf.h"
#include "err.h"

#define ARGB32_DEPTH    32
#define ARGB_RED_MASK   0x00ff0000
#define ARGB_GREEN_MASK 0x0000ff00
#define ARGB_BLUE_MASK  0x000000ff

static struct {
    Display        *display;
    Window          window;
    GC              gc;
    XShmSegmentInfo shm;
    XImage         *img;
    int             logical_w, logical_h;   /* Rendering resolution (what app sees) */
    int             physical_w, physical_h; /* Window/display resolution (what backend shows) */
    Atom            wm_delete;
    /* Input method / input context: required for Xutf8LookupString to
     * produce correct UTF-8 text (dead keys, compose sequences, non-Latin1
     * layouts). Without this, XLookupString would only yield raw Latin-1
     * bytes for accented characters, which is NOT valid UTF-8. */
    XIM             xim;
    XIC             xic;
    /* Text input buffer for typed characters (UTF-8) */
    char            text_buffer[128];
    int             text_buffer_len;
} g_x11;

static bool
select_argb_visual(Display *display, Visual **out_visual, int *out_depth) {
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
pxl_backend_init(const char *title, int w, int h, pxl_backend_flags_t flags) {
    pxl_backend_deinit();

    if (!title || w <= 0 || h <= 0) {
        return PXL_E_INVALID_PARAM;
    }

    /* Required for Xutf8LookupString to produce correct UTF-8 output and
     * for XIM to negotiate a proper input method with the OS/desktop. */
    setlocale(LC_CTYPE, "");
    XSetLocaleModifiers("");

    g_x11.display = XOpenDisplay(NULL);
    if (!g_x11.display) {
        pxl_log("XOpenDisplay failed");
        return PXL_E_BACKEND_INIT;
    }

    Visual *visual = NULL;
    int depth = 0;
    if (!select_argb_visual(g_x11.display, &visual, &depth)) goto fail;

    int scr = DefaultScreen(g_x11.display);
    Window root = RootWindow(g_x11.display, scr);
    Colormap cmap = XCreateColormap(g_x11.display, root, visual, AllocNone);

    XSetWindowAttributes attrs = {
        .colormap = cmap,
        .background_pixel = 0,
        .border_pixel = 0,
    };

    /* Calculate position for centered window */
    int x = 0, y = 0;
    if (flags & PXL_BACKEND_CENTERED) {
        x = (DisplayWidth(g_x11.display, scr) - w) / 2;
        y = (DisplayHeight(g_x11.display, scr) - h) / 2;
    }

    g_x11.window = XCreateWindow(
        g_x11.display, root,
        x, y, (unsigned int)w, (unsigned int)h, 0u,
        depth, InputOutput, visual,
        CWColormap | CWBackPixel | CWBorderPixel,
        &attrs
    );
    if (!g_x11.window) goto fail;

    XStoreName(g_x11.display, g_x11.window, title);
    XSelectInput(g_x11.display, g_x11.window,
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask);
	
	XkbSetDetectableAutoRepeat(g_x11.display, True, NULL);

    g_x11.wm_delete = XInternAtom(g_x11.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_x11.display, g_x11.window, &g_x11.wm_delete, 1);

    g_x11.xim = XOpenIM(g_x11.display, NULL, NULL, NULL);
    if (!g_x11.xim) goto fail;

    g_x11.xic = XCreateIC(g_x11.xim,
        XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow, g_x11.window,
        XNFocusWindow, g_x11.window,
        NULL);
    if (!g_x11.xic) goto fail;

    /* Map window temporarily for XShmAttach to work */
    XMapWindow(g_x11.display, g_x11.window);
    XSync(g_x11.display, False);

    g_x11.img = XShmCreateImage(g_x11.display, visual, (unsigned int)depth, ZPixmap, NULL,
                                &g_x11.shm, (unsigned int)w, (unsigned int)h);
    if (!g_x11.img) goto fail;

	/* ensure we are pixel-aligned */
    if (g_x11.img->bytes_per_line % (int)sizeof(pxl_t) != 0) goto fail;

	/* Prevent integer overflow in shared memory size calculation */
	if (h > 0 && g_x11.img->bytes_per_line > INT_MAX / h) {
		goto fail;
	}

    g_x11.shm.shmid = shmget(IPC_PRIVATE,
                             (size_t)g_x11.img->bytes_per_line * (size_t)h,
                             IPC_CREAT | 0777);
    if (g_x11.shm.shmid < 0) goto fail;

    g_x11.shm.shmaddr = g_x11.img->data = shmat(g_x11.shm.shmid, 0, 0);
    if (g_x11.shm.shmaddr == (char *)-1) goto fail;

    g_x11.shm.readOnly = False;

    if (!XShmAttach(g_x11.display, &g_x11.shm)) goto fail;

    g_x11.gc = XCreateGC(g_x11.display, g_x11.window, 0, NULL);
    if (!g_x11.gc) goto fail;

    g_x11.logical_w = w;
    g_x11.logical_h = h;

    /* Unmap if hidden flag is set */
    if (flags & PXL_BACKEND_HIDDEN) {
        XUnmapWindow(g_x11.display, g_x11.window);
        XSync(g_x11.display, False);
    }

    /* Fullscreen via EWMH */
    if (flags & PXL_BACKEND_FULLSCREEN) {
        Atom wm_state = XInternAtom(g_x11.display, "_NET_WM_STATE", False);
        Atom fullscreen_atom = XInternAtom(g_x11.display, "_NET_WM_STATE_FULLSCREEN", False);

        XEvent e = {.xclient = {
            .type = ClientMessage,
            .serial = 0,
            .send_event = True,
            .display = g_x11.display,
            .window = g_x11.window,
            .message_type = wm_state,
            .format = 32,
            .data.l = {1L, (long)fullscreen_atom, 0L, 0L, 0L}
        }};
        XSendEvent(g_x11.display, RootWindow(g_x11.display, DefaultScreen(g_x11.display)),
                  False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
    }

    XFlush(g_x11.display);
    
    /* Initialize physical size to match logical size */
    g_x11.physical_w = w;
    g_x11.physical_h = h;


    return PXL_SUCCESS;

fail:
    pxl_backend_deinit();
    return PXL_E_BACKEND_INIT;
}

void
pxl_backend_deinit(void) {
    if (!g_x11.display) return;

    if (g_x11.img) {
        XShmDetach(g_x11.display, &g_x11.shm);
        if (g_x11.shm.shmaddr && g_x11.shm.shmaddr != (char *)-1) {
            shmdt(g_x11.shm.shmaddr);
        }
        shmctl(g_x11.shm.shmid, IPC_RMID, NULL);
        XDestroyImage(g_x11.img);
        g_x11.img = NULL;
    }

    if (g_x11.gc) {
        XFreeGC(g_x11.display, g_x11.gc);
        g_x11.gc = 0;
    }

    if (g_x11.xic) {
        XDestroyIC(g_x11.xic);
        g_x11.xic = NULL;
    }
    if (g_x11.xim) {
        XCloseIM(g_x11.xim);
        g_x11.xim = NULL;
    }

    if (g_x11.window) {
        XDestroyWindow(g_x11.display, g_x11.window);
        g_x11.window = 0;
    }

    g_x11.text_buffer_len = 0;

    XCloseDisplay(g_x11.display);
    g_x11.display = NULL;
}

pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb) {
	assert(out_pb);
	assert(g_x11.display && g_x11.img && g_x11.img->data);
    assert(g_x11.img->bytes_per_line % (int)sizeof(pxl_t) == 0);

    /* Always return LOGICAL size to the application */
    out_pb->width  = g_x11.logical_w;
    out_pb->height = g_x11.logical_h;
    out_pb->stride = g_x11.img->bytes_per_line / (int)sizeof(pxl_t);
    out_pb->data   = (pxl_t *)g_x11.img->data;

    return PXL_SUCCESS;
}

pxl_err_t
pxl_backend_end_frame(void) {
    assert(g_x11.display && g_x11.img && g_x11.img->data);

    Bool success = XShmPutImage(g_x11.display, g_x11.window, g_x11.gc,
                                g_x11.img, 0, 0, 0, 0, (unsigned int)g_x11.logical_w, (unsigned int)g_x11.logical_h,
                                False);
    XSync(g_x11.display, False);
    
    if (!success) {
        pxl_log("XShmPutImage failed");
    }
    return success ? PXL_SUCCESS : PXL_E_BACKEND_FRAME;
}

double
pxl_backend_get_time(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static pxl_input_code_t
x11_keysym_to_pxl_input_code(const KeySym sym) {
    switch (sym) {
        case XK_Escape: return PXL_KEYB_ESCAPE;
        case XK_Return: return PXL_KEYB_ENTER;
        case XK_Tab: return PXL_KEYB_TAB;
        case XK_BackSpace: return PXL_KEYB_BACKSPACE;
        case XK_Insert: return PXL_KEYB_INSERT;
        case XK_Delete: return PXL_KEYB_DELETE;
        case XK_Home: return PXL_KEYB_HOME;
        case XK_End: return PXL_KEYB_END;
        case XK_Page_Up: return PXL_KEYB_PAGE_UP;
        case XK_Page_Down: return PXL_KEYB_PAGE_DOWN;

        case XK_Up: return PXL_KEYB_UP;
        case XK_Down: return PXL_KEYB_DOWN;
        case XK_Left: return PXL_KEYB_LEFT;
        case XK_Right: return PXL_KEYB_RIGHT;

        case XK_Shift_L: return PXL_KEYB_LSHIFT;
        case XK_Shift_R: return PXL_KEYB_RSHIFT;
        case XK_Control_L: return PXL_KEYB_LCTRL;
        case XK_Control_R: return PXL_KEYB_RCTRL;
        case XK_Alt_L: return PXL_KEYB_LALT;
        case XK_Alt_R: return PXL_KEYB_RALT;
        case XK_Super_L: return PXL_KEYB_LSUPER;
        case XK_Super_R: return PXL_KEYB_RSUPER;

        case XK_space: return PXL_KEYB_SPACE;
        case XK_comma: return PXL_KEYB_COMMA;
        case XK_period: return PXL_KEYB_PERIOD;
        case XK_slash: return PXL_KEYB_SLASH;
        case XK_semicolon: return PXL_KEYB_SEMICOLON;
        case XK_equal: return PXL_KEYB_EQUAL;
        case XK_bracketleft: return PXL_KEYB_LEFT_BRACKET;
        case XK_backslash: return PXL_KEYB_BACKSLASH;
        case XK_bracketright: return PXL_KEYB_RIGHT_BRACKET;
        case XK_grave: return PXL_KEYB_GRAVE_ACCENT;

        /* Numbers: 0-9 with AZERTY/US/Shift/AltGr variants */
        case XK_0: case XK_parenright: case XK_agrave: return PXL_KEYB_0;
        case XK_1: case XK_exclam: case XK_ampersand: return PXL_KEYB_1;
        case XK_2: case XK_at: case XK_eacute: return PXL_KEYB_2;
        case XK_3: case XK_numbersign: case XK_quotedbl: return PXL_KEYB_3;
        case XK_4: case XK_dollar: case XK_apostrophe: return PXL_KEYB_4;
        case XK_5: case XK_percent: case XK_parenleft: return PXL_KEYB_5;
        case XK_6: case XK_asciicircum: case XK_minus: return PXL_KEYB_6;
        case XK_7: case XK_egrave: return PXL_KEYB_7;
        case XK_8: case XK_asterisk: case XK_underscore: return PXL_KEYB_8;
        case XK_9: case XK_ccedilla: return PXL_KEYB_9;

        case XK_a: case XK_A: return PXL_KEYB_A;
        case XK_b: case XK_B: return PXL_KEYB_B;
        case XK_c: case XK_C: return PXL_KEYB_C;
        case XK_d: case XK_D: return PXL_KEYB_D;
        case XK_e: case XK_E: return PXL_KEYB_E;
        case XK_f: case XK_F: return PXL_KEYB_F;
        case XK_g: case XK_G: return PXL_KEYB_G;
        case XK_h: case XK_H: return PXL_KEYB_H;
        case XK_i: case XK_I: return PXL_KEYB_I;
        case XK_j: case XK_J: return PXL_KEYB_J;
        case XK_k: case XK_K: return PXL_KEYB_K;
        case XK_l: case XK_L: return PXL_KEYB_L;
        case XK_m: case XK_M: return PXL_KEYB_M;
        case XK_n: case XK_N: return PXL_KEYB_N;
        case XK_o: case XK_O: return PXL_KEYB_O;
        case XK_p: case XK_P: return PXL_KEYB_P;
        case XK_q: case XK_Q: return PXL_KEYB_Q;
        case XK_r: case XK_R: return PXL_KEYB_R;
        case XK_s: case XK_S: return PXL_KEYB_S;
        case XK_t: case XK_T: return PXL_KEYB_T;
        case XK_u: case XK_U: return PXL_KEYB_U;
        case XK_v: case XK_V: return PXL_KEYB_V;
        case XK_w: case XK_W: return PXL_KEYB_W;
        case XK_x: case XK_X: return PXL_KEYB_X;
        case XK_y: case XK_Y: return PXL_KEYB_Y;
        case XK_z: case XK_Z: return PXL_KEYB_Z;

        case XK_F1: return PXL_KEYB_F1; case XK_F2: return PXL_KEYB_F2; case XK_F3: return PXL_KEYB_F3;
        case XK_F4: return PXL_KEYB_F4; case XK_F5: return PXL_KEYB_F5; case XK_F6: return PXL_KEYB_F6;
        case XK_F7: return PXL_KEYB_F7; case XK_F8: return PXL_KEYB_F8; case XK_F9: return PXL_KEYB_F9;
        case XK_F10: return PXL_KEYB_F10; case XK_F11: return PXL_KEYB_F11; case XK_F12: return PXL_KEYB_F12;
        case XK_F13: return PXL_KEYB_F13; case XK_F14: return PXL_KEYB_F14; case XK_F15: return PXL_KEYB_F15;
        case XK_F16: return PXL_KEYB_F16; case XK_F17: return PXL_KEYB_F17; case XK_F18: return PXL_KEYB_F18;
        case XK_F19: return PXL_KEYB_F19; case XK_F20: return PXL_KEYB_F20; case XK_F21: return PXL_KEYB_F21;
        case XK_F22: return PXL_KEYB_F22; case XK_F23: return PXL_KEYB_F23; case XK_F24: return PXL_KEYB_F24;

        case XK_KP_0: return PXL_KEYB_KP_0; case XK_KP_1: return PXL_KEYB_KP_1; case XK_KP_2: return PXL_KEYB_KP_2;
        case XK_KP_3: return PXL_KEYB_KP_3; case XK_KP_4: return PXL_KEYB_KP_4; case XK_KP_5: return PXL_KEYB_KP_5;
        case XK_KP_6: return PXL_KEYB_KP_6; case XK_KP_7: return PXL_KEYB_KP_7; case XK_KP_8: return PXL_KEYB_KP_8;
        case XK_KP_9: return PXL_KEYB_KP_9;
        case XK_KP_Decimal: return PXL_KEYB_KP_DECIMAL;
        case XK_KP_Divide: return PXL_KEYB_KP_DIVIDE;
        case XK_KP_Multiply: return PXL_KEYB_KP_MULTIPLY;
        case XK_KP_Subtract: return PXL_KEYB_KP_SUBTRACT;
        case XK_KP_Add: return PXL_KEYB_KP_ADD;
        case XK_KP_Enter: return PXL_KEYB_KP_ENTER;
        case XK_KP_Equal: return PXL_KEYB_KP_EQUAL;

        default: return PXL_IN_UNKNOWN;
    }
}

static pxl_input_code_t
x11_button_to_pxl_input_code(const unsigned int button) {
    switch (button) {
        case Button1: return PXL_MOUSE_LEFT;
        case Button2: return PXL_MOUSE_MIDDLE;
        case Button3: return PXL_MOUSE_RIGHT;
        default: return PXL_IN_UNKNOWN;
    }
}

/* Process a single X11 event and update input state */
static void
process_x11_event(XEvent *event, pxl_input_t *in) {
    switch (event->type) {
        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == g_x11.wm_delete) {
                pxl_input_press(in, PXL_WM_QUIT);
            }
            break;

        case KeyPress: {
            /* Get the physical key code */
            pxl_input_press(in, x11_keysym_to_pxl_input_code(XLookupKeysym(&event->xkey, 0)));

            /* Get the actual character from the OS keyboard layout / IME using
             * Xutf8LookupString. */
            char buf[32];
            KeySym keysym_return;
            Status status;
            int len = Xutf8LookupString(g_x11.xic, &event->xkey, buf, sizeof(buf) - 1,
                                         &keysym_return, &status);
            if (len > 0 && (status == XLookupChars || status == XLookupBoth)) {
                /* FIFO: if buffer is full, shift left to make room for new characters */
                if (g_x11.text_buffer_len + len > (int)sizeof(g_x11.text_buffer)) {
                    int excess = (g_x11.text_buffer_len + len) - (int)sizeof(g_x11.text_buffer);
                    memmove(g_x11.text_buffer, g_x11.text_buffer + excess, (size_t)(g_x11.text_buffer_len - excess));
                    g_x11.text_buffer_len -= excess;
                }
                memcpy(g_x11.text_buffer + g_x11.text_buffer_len, buf, (size_t)len);
                g_x11.text_buffer_len += len;
            }
            break;
        }

        case KeyRelease:
            pxl_input_release(in, x11_keysym_to_pxl_input_code(XLookupKeysym(&event->xkey, 0)));
            break;

        case ButtonPress: {
            pxl_input_code_t b = x11_button_to_pxl_input_code(event->xbutton.button);
            if (b != PXL_IN_UNKNOWN) {
				pxl_input_press(in, b);
            } else if (event->xbutton.button == 4) {
                in->mouse_wheel_y += 1;
            } else if (event->xbutton.button == 5) {
                in->mouse_wheel_y -= 1;
            } else if (event->xbutton.button == 6) {
                in->mouse_wheel_x -= 1;
            } else if (event->xbutton.button == 7) {
                in->mouse_wheel_x += 1;
            }
            break;
        }

        case ButtonRelease:
			pxl_input_release(in, x11_button_to_pxl_input_code(event->xbutton.button));
            break;

        case MotionNotify:
            in->mouse_x = event->xmotion.x;
            in->mouse_y = event->xmotion.y;
            break;

        case EnterNotify:
            pxl_input_release(in, PXL_WM_MOUSE_FOCUS_LOST);
            in->mouse_x = event->xcrossing.x;
            in->mouse_y = event->xcrossing.y;
            break;

        case LeaveNotify:
            pxl_input_press(in, PXL_WM_MOUSE_FOCUS_LOST);
            break;

        case FocusIn:
            pxl_input_release(in, PXL_WM_FOCUS_LOST);
            if (g_x11.xic) XSetICFocus(g_x11.xic);
            break;

        case FocusOut:
            pxl_input_press(in, PXL_WM_FOCUS_LOST);
            if (g_x11.xic) XUnsetICFocus(g_x11.xic);
            break;
    }
}

void
pxl_backend_poll_events(pxl_input_t *in) {
    XEvent event;
    while (XPending(g_x11.display)) {
        XNextEvent(g_x11.display, &event);
        /* Events consumed by the input method (e.g. mid-compose dead-key
         * sequences) must not be processed as normal key events. */
        if (XFilterEvent(&event, None)) continue;
        process_x11_event(&event, in);
    }
}

void
pxl_backend_wait_events(pxl_input_t *in) {
    XEvent event;
    XNextEvent(g_x11.display, &event); /* Block until first event */
    if (!XFilterEvent(&event, None)) {
        process_x11_event(&event, in);
    }
    while (XPending(g_x11.display)) {
        XNextEvent(g_x11.display, &event);
        if (XFilterEvent(&event, None)) continue;
        process_x11_event(&event, in);
    }
}

int
pxl_backend_get_typed_text(char *out_text, int out_text_max_len) {
    assert(out_text);
    assert(out_text_max_len > 0);

    if (g_x11.text_buffer_len == 0) return 0;

    int copy_len = (g_x11.text_buffer_len < out_text_max_len)
        ? g_x11.text_buffer_len
        : out_text_max_len - 1;

    /* Early return if nothing to copy (kept for clarity and to avoid useless operations) */
    if (copy_len <= 0) return 0;

    memcpy(out_text, g_x11.text_buffer, (size_t)copy_len);
    out_text[copy_len] = '\0';

    /* Consume copied bytes (no null-termination in internal buffer) */
    g_x11.text_buffer_len -= copy_len;
    memmove(g_x11.text_buffer, g_x11.text_buffer + copy_len, (size_t)g_x11.text_buffer_len);

    return copy_len;
}

void
pxl_backend_set_physical_size(int physical_w, int physical_h) {
    if (physical_w <= 0 || physical_h <= 0) {
        return;  /* Ignore invalid sizes */
    }
    
    /* Update physical size */
    g_x11.physical_w = physical_w;
    g_x11.physical_h = physical_h;
    
    /* Resize window */
    XResizeWindow(g_x11.display, g_x11.window, (unsigned int)physical_w, (unsigned int)physical_h);
    XSync(g_x11.display, False);
}

void
pxl_backend_get_physical_size(int *out_w, int *out_h) {
    if (out_w) *out_w = g_x11.physical_w;
    if (out_h) *out_h = g_x11.physical_h;
}
