#include "backend.h"
#include "input.h"
#include "pixbuf.h"

#include <SDL.h>
#include <assert.h>
#include <stdint.h>

static struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    int width;
    int height;
} g_sdl;

pxl_err_t
backend_init(const char *title, int w, int h, bool fullscreen) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		return PXL_E_BACKEND_INIT;
	}

	uint32_t flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;
	g_sdl.window = SDL_CreateWindow(
		title,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		w, h,
		flags
	);
	if (!g_sdl.window) goto fail;

	// No VSYNC for minimal latency, TODO: consider adding an option if really needed.
	g_sdl.renderer = SDL_CreateRenderer(
		g_sdl.window, -1,
		SDL_RENDERER_ACCELERATED
	);
	if (!g_sdl.renderer) goto fail;

	g_sdl.texture = SDL_CreateTexture(
		g_sdl.renderer,
		SDL_PIXELFORMAT_ARGB8888,  // ARGB8888 to match PXL color native format
		SDL_TEXTUREACCESS_STREAMING,
		w, h
	);
	if (!g_sdl.texture) goto fail;

	g_sdl.width = w;
	g_sdl.height = h;
	return PXL_SUCCESS;

fail:
    backend_deinit();
    return PXL_E_BACKEND_INIT;
}

void
backend_deinit(void) {
    if (g_sdl.texture)  { SDL_DestroyTexture(g_sdl.texture); g_sdl.texture = NULL; }
    if (g_sdl.renderer) { SDL_DestroyRenderer(g_sdl.renderer); g_sdl.renderer = NULL; }
    if (g_sdl.window)   { SDL_DestroyWindow(g_sdl.window); g_sdl.window = NULL; }
    SDL_Quit();
}

pxl_err_t
backend_begin_frame(pixbuf_t *out_pb) {
    void *pixels;
    int pitch;

    if (SDL_LockTexture(g_sdl.texture, NULL, &pixels, &pitch) != 0) {
        return PXL_E_BACKEND_FRAME;
    }

    assert(pitch % (int)sizeof(pix_t) == 0);

    out_pb->width = g_sdl.width;
    out_pb->height = g_sdl.height;
    out_pb->stride = pitch / (int)sizeof(pix_t);
    out_pb->data = (pix_t *)pixels;

    return PXL_SUCCESS;
}

void
backend_end_frame(void) {
    SDL_UnlockTexture(g_sdl.texture);
    SDL_RenderClear(g_sdl.renderer);
    SDL_RenderCopy(g_sdl.renderer, g_sdl.texture, NULL, NULL);
    SDL_RenderPresent(g_sdl.renderer);
}

double
backend_get_time(void) {
    uint64_t ticks = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    return (double)ticks / (double)freq;
}

static input_code_t
sdl_keysym_to_input_code(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_ESCAPE: return IN_KEYB_ESCAPE;
        case SDLK_RETURN: return IN_KEYB_ENTER;
        case SDLK_TAB: return IN_KEYB_TAB;
        case SDLK_BACKSPACE: return IN_KEYB_BACKSPACE;
        case SDLK_INSERT: return IN_KEYB_INSERT;
        case SDLK_DELETE: return IN_KEYB_DELETE;
        case SDLK_HOME: return IN_KEYB_HOME;
        case SDLK_END: return IN_KEYB_END;
        case SDLK_PAGEUP: return IN_KEYB_PAGE_UP;
        case SDLK_PAGEDOWN: return IN_KEYB_PAGE_DOWN;

        case SDLK_UP: return IN_KEYB_UP;
        case SDLK_DOWN: return IN_KEYB_DOWN;
        case SDLK_LEFT: return IN_KEYB_LEFT;
        case SDLK_RIGHT: return IN_KEYB_RIGHT;

        case SDLK_LSHIFT: return IN_KEYB_LSHIFT;
        case SDLK_RSHIFT: return IN_KEYB_RSHIFT;
        case SDLK_LCTRL: return IN_KEYB_LCTRL;
        case SDLK_RCTRL: return IN_KEYB_RCTRL;
        case SDLK_LALT: return IN_KEYB_LALT;
        case SDLK_RALT: return IN_KEYB_RALT;
        case SDLK_LGUI: return IN_KEYB_LSUPER;
        case SDLK_RGUI: return IN_KEYB_RSUPER;

        case SDLK_SPACE: return IN_KEYB_SPACE;
        case SDLK_QUOTE: return IN_KEYB_APOSTROPHE;
        case SDLK_COMMA: return IN_KEYB_COMMA;
        case SDLK_MINUS: return IN_KEYB_MINUS;
        case SDLK_PERIOD: return IN_KEYB_PERIOD;
        case SDLK_SLASH: return IN_KEYB_SLASH;
        case SDLK_SEMICOLON: return IN_KEYB_SEMICOLON;
        case SDLK_EQUALS: return IN_KEYB_EQUAL;
        case SDLK_LEFTBRACKET: return IN_KEYB_LEFT_BRACKET;
        case SDLK_BACKSLASH: return IN_KEYB_BACKSLASH;
        case SDLK_RIGHTBRACKET: return IN_KEYB_RIGHT_BRACKET;
        case SDLK_BACKQUOTE: return IN_KEYB_GRAVE_ACCENT;

        case SDLK_0: return IN_KEYB_0;
        case SDLK_1: return IN_KEYB_1;
        case SDLK_2: return IN_KEYB_2;
        case SDLK_3: return IN_KEYB_3;
        case SDLK_4: return IN_KEYB_4;
        case SDLK_5: return IN_KEYB_5;
        case SDLK_6: return IN_KEYB_6;
        case SDLK_7: return IN_KEYB_7;
        case SDLK_8: return IN_KEYB_8;
        case SDLK_9: return IN_KEYB_9;

        case SDLK_a: return IN_KEYB_A;
        case SDLK_b: return IN_KEYB_B;
        case SDLK_c: return IN_KEYB_C;
        case SDLK_d: return IN_KEYB_D;
        case SDLK_e: return IN_KEYB_E;
        case SDLK_f: return IN_KEYB_F;
        case SDLK_g: return IN_KEYB_G;
        case SDLK_h: return IN_KEYB_H;
        case SDLK_i: return IN_KEYB_I;
        case SDLK_j: return IN_KEYB_J;
        case SDLK_k: return IN_KEYB_K;
        case SDLK_l: return IN_KEYB_L;
        case SDLK_m: return IN_KEYB_M;
        case SDLK_n: return IN_KEYB_N;
        case SDLK_o: return IN_KEYB_O;
        case SDLK_p: return IN_KEYB_P;
        case SDLK_q: return IN_KEYB_Q;
        case SDLK_r: return IN_KEYB_R;
        case SDLK_s: return IN_KEYB_S;
        case SDLK_t: return IN_KEYB_T;
        case SDLK_u: return IN_KEYB_U;
        case SDLK_v: return IN_KEYB_V;
        case SDLK_w: return IN_KEYB_W;
        case SDLK_x: return IN_KEYB_X;
        case SDLK_y: return IN_KEYB_Y;
        case SDLK_z: return IN_KEYB_Z;

        case SDLK_F1: return IN_KEYB_F1;
        case SDLK_F2: return IN_KEYB_F2;
        case SDLK_F3: return IN_KEYB_F3;
        case SDLK_F4: return IN_KEYB_F4;
        case SDLK_F5: return IN_KEYB_F5;
        case SDLK_F6: return IN_KEYB_F6;
        case SDLK_F7: return IN_KEYB_F7;
        case SDLK_F8: return IN_KEYB_F8;
        case SDLK_F9: return IN_KEYB_F9;
        case SDLK_F10: return IN_KEYB_F10;
        case SDLK_F11: return IN_KEYB_F11;
        case SDLK_F12: return IN_KEYB_F12;
        case SDLK_F13: return IN_KEYB_F13;
        case SDLK_F14: return IN_KEYB_F14;
        case SDLK_F15: return IN_KEYB_F15;
        case SDLK_F16: return IN_KEYB_F16;
        case SDLK_F17: return IN_KEYB_F17;
        case SDLK_F18: return IN_KEYB_F18;
        case SDLK_F19: return IN_KEYB_F19;
        case SDLK_F20: return IN_KEYB_F20;
        case SDLK_F21: return IN_KEYB_F21;
        case SDLK_F22: return IN_KEYB_F22;
        case SDLK_F23: return IN_KEYB_F23;
        case SDLK_F24: return IN_KEYB_F24;

        case SDLK_KP_0: return IN_KEYB_KP_0;
        case SDLK_KP_1: return IN_KEYB_KP_1;
        case SDLK_KP_2: return IN_KEYB_KP_2;
        case SDLK_KP_3: return IN_KEYB_KP_3;
        case SDLK_KP_4: return IN_KEYB_KP_4;
        case SDLK_KP_5: return IN_KEYB_KP_5;
        case SDLK_KP_6: return IN_KEYB_KP_6;
        case SDLK_KP_7: return IN_KEYB_KP_7;
        case SDLK_KP_8: return IN_KEYB_KP_8;
        case SDLK_KP_9: return IN_KEYB_KP_9;
        case SDLK_KP_PERIOD: return IN_KEYB_KP_DECIMAL;
        case SDLK_KP_DIVIDE: return IN_KEYB_KP_DIVIDE;
        case SDLK_KP_MULTIPLY: return IN_KEYB_KP_MULTIPLY;
        case SDLK_KP_MINUS: return IN_KEYB_KP_SUBTRACT;
        case SDLK_KP_PLUS: return IN_KEYB_KP_ADD;
        case SDLK_KP_ENTER: return IN_KEYB_KP_ENTER;
        case SDLK_KP_EQUALS: return IN_KEYB_KP_EQUAL;

        default: return IN_UNKNOWN;
    }
}

static input_code_t
sdl_button_to_input_code(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:   return IN_MOUSE_LEFT;
        case SDL_BUTTON_RIGHT:  return IN_MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return IN_MOUSE_MIDDLE;
        default: return IN_UNKNOWN;
    }
}

void
backend_poll_events(input_state_t *in) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
				input_press(in, IN_WM_QUIT);
                break;

            case SDL_KEYDOWN:
                if (!event.key.repeat) {
					input_press(in, sdl_keysym_to_input_code(event.key.keysym.sym));
                }
                break;

            case SDL_KEYUP:
				input_release(in, sdl_keysym_to_input_code(event.key.keysym.sym));
                break;

            case SDL_MOUSEBUTTONDOWN:
				input_press(in, sdl_button_to_input_code(event.key.keysym.sym));
                break;

            case SDL_MOUSEBUTTONUP:
				input_release(in, sdl_button_to_input_code(event.key.keysym.sym));
                break;

            case SDL_MOUSEMOTION:
                input_set_mouse_pos(in, event.motion.x, event.motion.y);
                break;

            case SDL_MOUSEWHEEL:
                input_inc_mouse_wheel(in, event.wheel.x, event.wheel.y);
                break;
        }
    }
}
