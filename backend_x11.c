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
#include "buf.h"

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
pxl_backend_init(const char *title, int w, int h, bool fullscreen) {
    (void)fullscreen;        /* TODO */

    pxl_backend_deinit();

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

	
	XkbSetDetectableAutoRepeat(g_x11.display, True, NULL);

    g_x11.wm_delete = XInternAtom(g_x11.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_x11.display, g_x11.window, &g_x11.wm_delete, 1);

    g_x11.img = XShmCreateImage(g_x11.display, visual, depth, ZPixmap, NULL,
                                &g_x11.shm, w, h);
    if (!g_x11.img) goto fail;

	/* ensure we are pixel-aligned */
    if (g_x11.img->bytes_per_line % (int)sizeof(pxl_t) != 0) goto fail;

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

    if (g_x11.window) {
        XDestroyWindow(g_x11.display, g_x11.window);
        g_x11.window = 0;
    }

    XCloseDisplay(g_x11.display);
    g_x11.display = NULL;
}

pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb) {
    assert(g_x11.display && g_x11.img && g_x11.img->data);
    assert(g_x11.img->bytes_per_line % (int)sizeof(pxl_t) == 0);

    out_pb->width  = g_x11.width;
    out_pb->height = g_x11.height;
    out_pb->stride = g_x11.img->bytes_per_line / (int)sizeof(pxl_t);
    out_pb->data   = (pxl_t *)g_x11.img->data;

    return PXL_SUCCESS;
}

void
pxl_backend_end_frame(void) {
    assert(g_x11.display && g_x11.img && g_x11.img->data);

    XShmPutImage(g_x11.display, g_x11.window, g_x11.gc,
                 g_x11.img, 0, 0, 0, 0, g_x11.width, g_x11.height,
                 False);
    XSync(g_x11.display, False);
}

double
pxl_backend_get_time(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0.0;
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static pxl_input_code_t
x11_keysym_to_pxl_input_code(KeySym sym) {
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
        case XK_apostrophe: return PXL_KEYB_APOSTROPHE;
        case XK_comma: return PXL_KEYB_COMMA;
        case XK_minus: return PXL_KEYB_MINUS;
        case XK_period: return PXL_KEYB_PERIOD;
        case XK_slash: return PXL_KEYB_SLASH;
        case XK_semicolon: return PXL_KEYB_SEMICOLON;
        case XK_equal: return PXL_KEYB_EQUAL;
        case XK_bracketleft: return PXL_KEYB_LEFT_BRACKET;
        case XK_backslash: return PXL_KEYB_BACKSLASH;
        case XK_bracketright: return PXL_KEYB_RIGHT_BRACKET;
        case XK_grave: return PXL_KEYB_GRAVE_ACCENT;

        case XK_0: return PXL_KEYB_0; case XK_1: return PXL_KEYB_1; case XK_2: return PXL_KEYB_2;
        case XK_3: return PXL_KEYB_3; case XK_4: return PXL_KEYB_4; case XK_5: return PXL_KEYB_5;
        case XK_6: return PXL_KEYB_6; case XK_7: return PXL_KEYB_7; case XK_8: return PXL_KEYB_8;
        case XK_9: return PXL_KEYB_9;

        case XK_a: return PXL_KEYB_A; case XK_b: return PXL_KEYB_B; case XK_c: return PXL_KEYB_C;
        case XK_d: return PXL_KEYB_D; case XK_e: return PXL_KEYB_E; case XK_f: return PXL_KEYB_F;
        case XK_g: return PXL_KEYB_G; case XK_h: return PXL_KEYB_H; case XK_i: return PXL_KEYB_I;
        case XK_j: return PXL_KEYB_J; case XK_k: return PXL_KEYB_K; case XK_l: return PXL_KEYB_L;
        case XK_m: return PXL_KEYB_M; case XK_n: return PXL_KEYB_N; case XK_o: return PXL_KEYB_O;
        case XK_p: return PXL_KEYB_P; case XK_q: return PXL_KEYB_Q; case XK_r: return PXL_KEYB_R;
        case XK_s: return PXL_KEYB_S; case XK_t: return PXL_KEYB_T; case XK_u: return PXL_KEYB_U;
        case XK_v: return PXL_KEYB_V; case XK_w: return PXL_KEYB_W; case XK_x: return PXL_KEYB_X;
        case XK_y: return PXL_KEYB_Y; case XK_z: return PXL_KEYB_Z;

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
x11_button_to_pxl_input_code(unsigned int button) {
    switch (button) {
        case Button1: return PXL_MOUSE_LEFT;
        case Button2: return PXL_MOUSE_MIDDLE;
        case Button3: return PXL_MOUSE_RIGHT;
        default: return PXL_IN_UNKNOWN;
    }
}

void
pxl_backend_poll_events(pxl_input_state_t *in) {
    XEvent event;

    while (XPending(g_x11.display)) {
        XNextEvent(g_x11.display, &event);

        switch (event.type) {
            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == g_x11.wm_delete) {
                    pxl_input_press(in, PXL_WM_QUIT);
                }
                break;

            case KeyPress:
                pxl_input_press(in, x11_keysym_to_pxl_input_code(XLookupKeysym(&event.xkey, 0)));
                break;

            case KeyRelease:
                pxl_input_release(in, x11_keysym_to_pxl_input_code(XLookupKeysym(&event.xkey, 0)));
                break;

            case ButtonPress: {
                pxl_input_code_t b = x11_button_to_pxl_input_code(event.xbutton.button);
                if (b != PXL_IN_UNKNOWN) {
					pxl_input_press(in, b);
                } else if (event.xbutton.button == 4) {
                    pxl_input_inc_mouse_wheel(in, 0, 1);
                } else if (event.xbutton.button == 5) {
                    pxl_input_inc_mouse_wheel(in, 0, -1);
                } else if (event.xbutton.button == 6) {
                    pxl_input_inc_mouse_wheel(in, -1, 0);
                } else if (event.xbutton.button == 7) {
                    pxl_input_inc_mouse_wheel(in, 1, 0);
                }
                break;
            }

            case ButtonRelease: {
				pxl_input_release(in, x11_button_to_pxl_input_code(event.xbutton.button));
                break;
            }

            case MotionNotify:
                pxl_input_set_mouse_pos(in, event.xmotion.x, event.xmotion.y);
                break;
        }
    }
}
