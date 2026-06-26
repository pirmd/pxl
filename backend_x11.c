#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "backend.h"
#include "input.h"
#include "pixbuf.h"

/* Only little-endian is supported - X11 uses ARGB8888 in memory */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "X11 backend only supports little-endian architectures"
#endif

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
    int             width, height;
    Atom            wm_delete;
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
backend_init(const char *title, int w, int h, bool fullscreen) {
    (void)fullscreen;        /* TODO */

    backend_deinit();

    g_x11.display = XOpenDisplay(NULL);
    if (!g_x11.display) return PXL_E_BACKEND_INIT;

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

    g_x11.window = XCreateWindow(
        g_x11.display, root,
        0, 0, w, h, 0,
        depth, InputOutput, visual,
        CWColormap | CWBackPixel | CWBorderPixel,
        &attrs
    );
    if (!g_x11.window) goto fail;

    XStoreName(g_x11.display, g_x11.window, title);
    XSelectInput(g_x11.display, g_x11.window,
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        StructureNotifyMask);

	Bool supported;
	XkbSetDetectableAutoRepeat(g_x11.display, True, &supported);

    g_x11.wm_delete = XInternAtom(g_x11.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_x11.display, g_x11.window, &g_x11.wm_delete, 1);

    g_x11.img = XShmCreateImage(g_x11.display, visual, depth, ZPixmap, NULL,
                                &g_x11.shm, w, h);
    if (!g_x11.img) goto fail;

	/* ensure we are pixel-aligned */
    if (g_x11.img->bytes_per_line % (int)sizeof(pix_t) != 0) goto fail;

    g_x11.shm.shmid = shmget(IPC_PRIVATE,
                             g_x11.img->bytes_per_line * h,
                             IPC_CREAT | 0777);
    if (g_x11.shm.shmid < 0) goto fail;

    g_x11.shm.shmaddr = g_x11.img->data = shmat(g_x11.shm.shmid, 0, 0);
    if (g_x11.shm.shmaddr == (char *)-1) goto fail;

    g_x11.shm.readOnly = False;

    if (!XShmAttach(g_x11.display, &g_x11.shm)) goto fail;

    g_x11.gc = XCreateGC(g_x11.display, g_x11.window, 0, NULL);
    if (!g_x11.gc) goto fail;

    g_x11.width = w;
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

    if (g_x11.window) {
        XDestroyWindow(g_x11.display, g_x11.window);
        g_x11.window = 0;
    }

    XCloseDisplay(g_x11.display);
    g_x11.display = NULL;
}

pxl_err_t
backend_begin_frame(pixbuf_t *out_pb) {
    assert(g_x11.display && g_x11.img && g_x11.img->data);
    assert(g_x11.img->bytes_per_line % (int)sizeof(pix_t) == 0);

    out_pb->width  = g_x11.width;
    out_pb->height = g_x11.height;
    out_pb->stride = g_x11.img->bytes_per_line / (int)sizeof(pix_t);
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

double
backend_get_time(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static key_code_t
x11_keysym_to_key(KeySym sym) {
    switch (sym) {
        case XK_Escape: return KEY_ESCAPE;
        case XK_Return: return KEY_ENTER;
        case XK_Tab: return KEY_TAB;
        case XK_BackSpace: return KEY_BACKSPACE;
        case XK_Insert: return KEY_INSERT;
        case XK_Delete: return KEY_DELETE;
        case XK_Home: return KEY_HOME;
        case XK_End: return KEY_END;
        case XK_Page_Up: return KEY_PAGE_UP;
        case XK_Page_Down: return KEY_PAGE_DOWN;

        case XK_Up: return KEY_UP;
        case XK_Down: return KEY_DOWN;
        case XK_Left: return KEY_LEFT;
        case XK_Right: return KEY_RIGHT;

        case XK_Shift_L: return KEY_LSHIFT;
        case XK_Shift_R: return KEY_RSHIFT;
        case XK_Control_L: return KEY_LCTRL;
        case XK_Control_R: return KEY_RCTRL;
        case XK_Alt_L: return KEY_LALT;
        case XK_Alt_R: return KEY_RALT;
        case XK_Super_L: return KEY_LSUPER;
        case XK_Super_R: return KEY_RSUPER;

        case XK_space: return KEY_SPACE;
        case XK_apostrophe: return KEY_APOSTROPHE;
        case XK_comma: return KEY_COMMA;
        case XK_minus: return KEY_MINUS;
        case XK_period: return KEY_PERIOD;
        case XK_slash: return KEY_SLASH;
        case XK_semicolon: return KEY_SEMICOLON;
        case XK_equal: return KEY_EQUAL;
        case XK_bracketleft: return KEY_LEFT_BRACKET;
        case XK_backslash: return KEY_BACKSLASH;
        case XK_bracketright: return KEY_RIGHT_BRACKET;
        case XK_grave: return KEY_GRAVE_ACCENT;

        case XK_0: return KEY_0; case XK_1: return KEY_1; case XK_2: return KEY_2;
        case XK_3: return KEY_3; case XK_4: return KEY_4; case XK_5: return KEY_5;
        case XK_6: return KEY_6; case XK_7: return KEY_7; case XK_8: return KEY_8;
        case XK_9: return KEY_9;

        case XK_a: return KEY_A; case XK_b: return KEY_B; case XK_c: return KEY_C;
        case XK_d: return KEY_D; case XK_e: return KEY_E; case XK_f: return KEY_F;
        case XK_g: return KEY_G; case XK_h: return KEY_H; case XK_i: return KEY_I;
        case XK_j: return KEY_J; case XK_k: return KEY_K; case XK_l: return KEY_L;
        case XK_m: return KEY_M; case XK_n: return KEY_N; case XK_o: return KEY_O;
        case XK_p: return KEY_P; case XK_q: return KEY_Q; case XK_r: return KEY_R;
        case XK_s: return KEY_S; case XK_t: return KEY_T; case XK_u: return KEY_U;
        case XK_v: return KEY_V; case XK_w: return KEY_W; case XK_x: return KEY_X;
        case XK_y: return KEY_Y; case XK_z: return KEY_Z;

        case XK_F1: return KEY_F1; case XK_F2: return KEY_F2; case XK_F3: return KEY_F3;
        case XK_F4: return KEY_F4; case XK_F5: return KEY_F5; case XK_F6: return KEY_F6;
        case XK_F7: return KEY_F7; case XK_F8: return KEY_F8; case XK_F9: return KEY_F9;
        case XK_F10: return KEY_F10; case XK_F11: return KEY_F11; case XK_F12: return KEY_F12;
        case XK_F13: return KEY_F13; case XK_F14: return KEY_F14; case XK_F15: return KEY_F15;
        case XK_F16: return KEY_F16; case XK_F17: return KEY_F17; case XK_F18: return KEY_F18;
        case XK_F19: return KEY_F19; case XK_F20: return KEY_F20; case XK_F21: return KEY_F21;
        case XK_F22: return KEY_F22; case XK_F23: return KEY_F23; case XK_F24: return KEY_F24;

        case XK_KP_0: return KEY_KP_0; case XK_KP_1: return KEY_KP_1; case XK_KP_2: return KEY_KP_2;
        case XK_KP_3: return KEY_KP_3; case XK_KP_4: return KEY_KP_4; case XK_KP_5: return KEY_KP_5;
        case XK_KP_6: return KEY_KP_6; case XK_KP_7: return KEY_KP_7; case XK_KP_8: return KEY_KP_8;
        case XK_KP_9: return KEY_KP_9;
        case XK_KP_Decimal: return KEY_KP_DECIMAL;
        case XK_KP_Divide: return KEY_KP_DIVIDE;
        case XK_KP_Multiply: return KEY_KP_MULTIPLY;
        case XK_KP_Subtract: return KEY_KP_SUBTRACT;
        case XK_KP_Add: return KEY_KP_ADD;
        case XK_KP_Enter: return KEY_KP_ENTER;
        case XK_KP_Equal: return KEY_KP_EQUAL;

        default: return KEY_UNKNOWN;
    }
}

static mouse_button_t
x11_button_to_mouse(unsigned int button) {
    switch (button) {
        case Button1: return MOUSE_LEFT;
        case Button2: return MOUSE_MIDDLE;
        case Button3: return MOUSE_RIGHT;
        default: return MOUSE_COUNT;
    }
}

void
backend_poll_events(input_t *input) {
    XEvent event;
    input_begin_frame(input);

    while (XPending(g_x11.display)) {
        XNextEvent(g_x11.display, &event);

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == g_x11.wm_delete) {
                    input->quit_requested = true;
                }
                break;

            case KeyPress:
                input->keys[x11_keysym_to_key(XLookupKeysym(&event.xkey, 0))] = true;
                break;

            case KeyRelease:
                input->keys[x11_keysym_to_key(XLookupKeysym(&event.xkey, 0))] = false;
                break;

            case ButtonPress: {
                mouse_button_t b = x11_button_to_mouse(event.xbutton.button);
                if (b < MOUSE_COUNT) {
                    input->mouse_buttons[b] = true;
                } else if (event.xbutton.button == 4) {
                    input_add_mouse_wheel(input, 0, 1);
                } else if (event.xbutton.button == 5) {
                    input_add_mouse_wheel(input, 0, -1);
                } else if (event.xbutton.button == 6) {
                    input_add_mouse_wheel(input, -1, 0);
                } else if (event.xbutton.button == 7) {
                    input_add_mouse_wheel(input, 1, 0);
                }
                break;
            }

            case ButtonRelease: {
                mouse_button_t b = x11_button_to_mouse(event.xbutton.button);
                if (b < MOUSE_COUNT) {
                    input->mouse_buttons[b] = false;
                }
                break;
            }

            case MotionNotify:
                input_set_mouse_pos(input, event.xmotion.x, event.xmotion.y);
                break;
        }
    }
}
