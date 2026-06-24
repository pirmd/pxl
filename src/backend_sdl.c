#include "backend.h"
#include "input.h"
#include <SDL.h>
#include <assert.h>
#include <stdint.h>

/* Backend state */
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

static key_code_t
sdl_keysym_to_key(SDL_Keycode sym) {
    switch (sym) {
        case SDLK_ESCAPE: return KEY_ESCAPE;
        case SDLK_RETURN: return KEY_ENTER;
        case SDLK_TAB: return KEY_TAB;
        case SDLK_BACKSPACE: return KEY_BACKSPACE;
        case SDLK_INSERT: return KEY_INSERT;
        case SDLK_DELETE: return KEY_DELETE;
        case SDLK_HOME: return KEY_HOME;
        case SDLK_END: return KEY_END;
        case SDLK_PAGEUP: return KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return KEY_PAGE_DOWN;

        case SDLK_UP: return KEY_UP;
        case SDLK_DOWN: return KEY_DOWN;
        case SDLK_LEFT: return KEY_LEFT;
        case SDLK_RIGHT: return KEY_RIGHT;

        case SDLK_LSHIFT: return KEY_LSHIFT;
        case SDLK_RSHIFT: return KEY_RSHIFT;
        case SDLK_LCTRL: return KEY_LCTRL;
        case SDLK_RCTRL: return KEY_RCTRL;
        case SDLK_LALT: return KEY_LALT;
        case SDLK_RALT: return KEY_RALT;
        case SDLK_LGUI: return KEY_LSUPER;
        case SDLK_RGUI: return KEY_RSUPER;

        case SDLK_SPACE: return KEY_SPACE;
        case SDLK_QUOTE: return KEY_APOSTROPHE;
        case SDLK_COMMA: return KEY_COMMA;
        case SDLK_MINUS: return KEY_MINUS;
        case SDLK_PERIOD: return KEY_PERIOD;
        case SDLK_SLASH: return KEY_SLASH;
        case SDLK_SEMICOLON: return KEY_SEMICOLON;
        case SDLK_EQUALS: return KEY_EQUAL;
        case SDLK_LEFTBRACKET: return KEY_LEFT_BRACKET;
        case SDLK_BACKSLASH: return KEY_BACKSLASH;
        case SDLK_RIGHTBRACKET: return KEY_RIGHT_BRACKET;
        case SDLK_BACKQUOTE: return KEY_GRAVE_ACCENT;

        case SDLK_0: return KEY_0;
        case SDLK_1: return KEY_1;
        case SDLK_2: return KEY_2;
        case SDLK_3: return KEY_3;
        case SDLK_4: return KEY_4;
        case SDLK_5: return KEY_5;
        case SDLK_6: return KEY_6;
        case SDLK_7: return KEY_7;
        case SDLK_8: return KEY_8;
        case SDLK_9: return KEY_9;

        case SDLK_a: return KEY_A;
        case SDLK_b: return KEY_B;
        case SDLK_c: return KEY_C;
        case SDLK_d: return KEY_D;
        case SDLK_e: return KEY_E;
        case SDLK_f: return KEY_F;
        case SDLK_g: return KEY_G;
        case SDLK_h: return KEY_H;
        case SDLK_i: return KEY_I;
        case SDLK_j: return KEY_J;
        case SDLK_k: return KEY_K;
        case SDLK_l: return KEY_L;
        case SDLK_m: return KEY_M;
        case SDLK_n: return KEY_N;
        case SDLK_o: return KEY_O;
        case SDLK_p: return KEY_P;
        case SDLK_q: return KEY_Q;
        case SDLK_r: return KEY_R;
        case SDLK_s: return KEY_S;
        case SDLK_t: return KEY_T;
        case SDLK_u: return KEY_U;
        case SDLK_v: return KEY_V;
        case SDLK_w: return KEY_W;
        case SDLK_x: return KEY_X;
        case SDLK_y: return KEY_Y;
        case SDLK_z: return KEY_Z;

        case SDLK_F1: return KEY_F1;
        case SDLK_F2: return KEY_F2;
        case SDLK_F3: return KEY_F3;
        case SDLK_F4: return KEY_F4;
        case SDLK_F5: return KEY_F5;
        case SDLK_F6: return KEY_F6;
        case SDLK_F7: return KEY_F7;
        case SDLK_F8: return KEY_F8;
        case SDLK_F9: return KEY_F9;
        case SDLK_F10: return KEY_F10;
        case SDLK_F11: return KEY_F11;
        case SDLK_F12: return KEY_F12;
        case SDLK_F13: return KEY_F13;
        case SDLK_F14: return KEY_F14;
        case SDLK_F15: return KEY_F15;
        case SDLK_F16: return KEY_F16;
        case SDLK_F17: return KEY_F17;
        case SDLK_F18: return KEY_F18;
        case SDLK_F19: return KEY_F19;
        case SDLK_F20: return KEY_F20;
        case SDLK_F21: return KEY_F21;
        case SDLK_F22: return KEY_F22;
        case SDLK_F23: return KEY_F23;
        case SDLK_F24: return KEY_F24;

        case SDLK_KP_0: return KEY_KP_0;
        case SDLK_KP_1: return KEY_KP_1;
        case SDLK_KP_2: return KEY_KP_2;
        case SDLK_KP_3: return KEY_KP_3;
        case SDLK_KP_4: return KEY_KP_4;
        case SDLK_KP_5: return KEY_KP_5;
        case SDLK_KP_6: return KEY_KP_6;
        case SDLK_KP_7: return KEY_KP_7;
        case SDLK_KP_8: return KEY_KP_8;
        case SDLK_KP_9: return KEY_KP_9;
        case SDLK_KP_PERIOD: return KEY_KP_DECIMAL;
        case SDLK_KP_DIVIDE: return KEY_KP_DIVIDE;
        case SDLK_KP_MULTIPLY: return KEY_KP_MULTIPLY;
        case SDLK_KP_MINUS: return KEY_KP_SUBTRACT;
        case SDLK_KP_PLUS: return KEY_KP_ADD;
        case SDLK_KP_ENTER: return KEY_KP_ENTER;
        case SDLK_KP_EQUALS: return KEY_KP_EQUAL;

        default: return KEY_UNKNOWN;
    }
}

static mouse_button_t
sdl_button_to_mouse(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:   return MOUSE_LEFT;
        case SDL_BUTTON_RIGHT:  return MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
        default: return MOUSE_COUNT;
    }
}

void
backend_poll_events(input_t *input) {
    SDL_Event event;

    input_begin_frame(input);

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                input->quit_requested = true;
                break;

            case SDL_KEYDOWN:
                if (!event.key.repeat) {
                    input->keys[sdl_keysym_to_key(event.key.keysym.sym)] = true;
                }
                break;

            case SDL_KEYUP:
                input->keys[sdl_keysym_to_key(event.key.keysym.sym)] = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
                input->mouse_buttons[sdl_button_to_mouse(event.button.button)] = true;
                break;

            case SDL_MOUSEBUTTONUP:
                input->mouse_buttons[sdl_button_to_mouse(event.button.button)] = false;
                break;

            case SDL_MOUSEMOTION:
                input_set_mouse_pos(input, event.motion.x, event.motion.y);
                break;

            case SDL_MOUSEWHEEL:
                input_add_mouse_wheel(input, event.wheel.x, event.wheel.y);
                break;
        }
    }
}
