#include "backend.h"
#include "input.h"
#include "buf.h"
#include "err.h"

#include <SDL.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

static struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    int width;
    int height;
    /* Text input buffer for typed characters (UTF-8) */
    char text_buffer[128];
    int text_buffer_len;
} g_sdl;

pxl_err_t
pxl_backend_init(const char *title, int w, int h, pxl_backend_flags_t flags) {
	pxl_backend_deinit();

	/* Validate parameters */
	if (!title || w <= 0 || h <= 0) {
		return PXL_E_INVALID_PARAM;
	}

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		pxl_log(SDL_GetError());
		return PXL_E_BACKEND_INIT;
	}

	/* Build window flags */
	uint32_t window_flags = 0;
	if (flags & PXL_BACKEND_FULLSCREEN) {
		window_flags |= SDL_WINDOW_FULLSCREEN;
	}
	if (flags & PXL_BACKEND_HIDDEN) {
		window_flags |= SDL_WINDOW_HIDDEN;
	}

	/* Calculate position for centered window */
	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
	if (flags & PXL_BACKEND_CENTERED) {
		x = SDL_WINDOWPOS_CENTERED;
		y = SDL_WINDOWPOS_CENTERED;
	}

	g_sdl.window = SDL_CreateWindow(
		title,
		x, y,
		w, h,
		window_flags
	);
	if (!g_sdl.window) goto fail;

	/* Build renderer flags */
	uint32_t renderer_flags = SDL_RENDERER_ACCELERATED;
	if (flags & PXL_BACKEND_VSYNC) {
		renderer_flags |= SDL_RENDERER_PRESENTVSYNC;
	}

	g_sdl.renderer = SDL_CreateRenderer(
		g_sdl.window, -1,
		renderer_flags
	);
	if (!g_sdl.renderer) goto fail;

	g_sdl.texture = SDL_CreateTexture(
		g_sdl.renderer,
		SDL_PIXELFORMAT_ARGB8888,  /* ARGB8888 to match PXL color native format */
		SDL_TEXTUREACCESS_STREAMING,
		w, h
	);
	if (!g_sdl.texture) goto fail;

	g_sdl.width = w;
	g_sdl.height = h;

	/* Enable text input for character retrieval */
	SDL_StartTextInput();

	return PXL_SUCCESS;

fail:
    pxl_backend_deinit();
    return PXL_E_BACKEND_INIT;
}

void
pxl_backend_deinit(void) {
    SDL_StopTextInput();
    g_sdl.text_buffer_len = 0;  /* No null-termination needed */
    if (g_sdl.texture)  { SDL_DestroyTexture(g_sdl.texture); g_sdl.texture = NULL; }
    if (g_sdl.renderer) { SDL_DestroyRenderer(g_sdl.renderer); g_sdl.renderer = NULL; }
    if (g_sdl.window)   { SDL_DestroyWindow(g_sdl.window); g_sdl.window = NULL; }
    SDL_Quit();
}

pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb) {
	assert(out_pb);

    void *pixels;
    int pitch;

    if (SDL_LockTexture(g_sdl.texture, NULL, &pixels, &pitch) != 0) {
        return PXL_E_BACKEND_FRAME;
    }

    assert(pitch % (int)sizeof(pxl_t) == 0);

    out_pb->width = g_sdl.width;
    out_pb->height = g_sdl.height;
    out_pb->stride = pitch / (int)sizeof(pxl_t);
    out_pb->data = (pxl_t *)pixels;

    return PXL_SUCCESS;
}

pxl_err_t
pxl_backend_end_frame(void) {
    SDL_UnlockTexture(g_sdl.texture);
    SDL_RenderClear(g_sdl.renderer);
    SDL_RenderCopy(g_sdl.renderer, g_sdl.texture, NULL, NULL);
    
    if (SDL_RenderPresent(g_sdl.renderer) != 0) {
        pxl_log(SDL_GetError());
        return PXL_E_BACKEND_FRAME;
    }
    return PXL_SUCCESS;
}

double
pxl_backend_get_time(void) {
    uint64_t ticks = SDL_GetPerformanceCounter();
    uint64_t freq = SDL_GetPerformanceFrequency();
    return (double)ticks / (double)freq;
}

static pxl_input_code_t
sdl_keysym_to_pxl_input_code(const SDL_Keycode sym) {
    switch (sym) {
        case SDLK_ESCAPE: return PXL_KEYB_ESCAPE;
        case SDLK_RETURN: return PXL_KEYB_ENTER;
        case SDLK_TAB: return PXL_KEYB_TAB;
        case SDLK_BACKSPACE: return PXL_KEYB_BACKSPACE;
        case SDLK_INSERT: return PXL_KEYB_INSERT;
        case SDLK_DELETE: return PXL_KEYB_DELETE;
        case SDLK_HOME: return PXL_KEYB_HOME;
        case SDLK_END: return PXL_KEYB_END;
        case SDLK_PAGEUP: return PXL_KEYB_PAGE_UP;
        case SDLK_PAGEDOWN: return PXL_KEYB_PAGE_DOWN;

        case SDLK_UP: return PXL_KEYB_UP;
        case SDLK_DOWN: return PXL_KEYB_DOWN;
        case SDLK_LEFT: return PXL_KEYB_LEFT;
        case SDLK_RIGHT: return PXL_KEYB_RIGHT;

        case SDLK_LSHIFT: return PXL_KEYB_LSHIFT;
        case SDLK_RSHIFT: return PXL_KEYB_RSHIFT;
        case SDLK_LCTRL: return PXL_KEYB_LCTRL;
        case SDLK_RCTRL: return PXL_KEYB_RCTRL;
        case SDLK_LALT: return PXL_KEYB_LALT;
        case SDLK_RALT: return PXL_KEYB_RALT;
        case SDLK_LGUI: return PXL_KEYB_LSUPER;
        case SDLK_RGUI: return PXL_KEYB_RSUPER;

        case SDLK_SPACE: return PXL_KEYB_SPACE;
        case SDLK_QUOTE: return PXL_KEYB_APOSTROPHE;
        case SDLK_COMMA: return PXL_KEYB_COMMA;
        case SDLK_MINUS: return PXL_KEYB_MINUS;
        case SDLK_PERIOD: return PXL_KEYB_PERIOD;
        case SDLK_SLASH: return PXL_KEYB_SLASH;
        case SDLK_SEMICOLON: return PXL_KEYB_SEMICOLON;
        case SDLK_EQUALS: return PXL_KEYB_EQUAL;
        case SDLK_LEFTBRACKET: return PXL_KEYB_LEFT_BRACKET;
        case SDLK_BACKSLASH: return PXL_KEYB_BACKSLASH;
        case SDLK_RIGHTBRACKET: return PXL_KEYB_RIGHT_BRACKET;
        case SDLK_BACKQUOTE: return PXL_KEYB_GRAVE_ACCENT;

        case SDLK_0: return PXL_KEYB_0;
        case SDLK_1: return PXL_KEYB_1;
        case SDLK_2: return PXL_KEYB_2;
        case SDLK_3: return PXL_KEYB_3;
        case SDLK_4: return PXL_KEYB_4;
        case SDLK_5: return PXL_KEYB_5;
        case SDLK_6: return PXL_KEYB_6;
        case SDLK_7: return PXL_KEYB_7;
        case SDLK_8: return PXL_KEYB_8;
        case SDLK_9: return PXL_KEYB_9;

        case SDLK_a: return PXL_KEYB_A;
        case SDLK_b: return PXL_KEYB_B;
        case SDLK_c: return PXL_KEYB_C;
        case SDLK_d: return PXL_KEYB_D;
        case SDLK_e: return PXL_KEYB_E;
        case SDLK_f: return PXL_KEYB_F;
        case SDLK_g: return PXL_KEYB_G;
        case SDLK_h: return PXL_KEYB_H;
        case SDLK_i: return PXL_KEYB_I;
        case SDLK_j: return PXL_KEYB_J;
        case SDLK_k: return PXL_KEYB_K;
        case SDLK_l: return PXL_KEYB_L;
        case SDLK_m: return PXL_KEYB_M;
        case SDLK_n: return PXL_KEYB_N;
        case SDLK_o: return PXL_KEYB_O;
        case SDLK_p: return PXL_KEYB_P;
        case SDLK_q: return PXL_KEYB_Q;
        case SDLK_r: return PXL_KEYB_R;
        case SDLK_s: return PXL_KEYB_S;
        case SDLK_t: return PXL_KEYB_T;
        case SDLK_u: return PXL_KEYB_U;
        case SDLK_v: return PXL_KEYB_V;
        case SDLK_w: return PXL_KEYB_W;
        case SDLK_x: return PXL_KEYB_X;
        case SDLK_y: return PXL_KEYB_Y;
        case SDLK_z: return PXL_KEYB_Z;

        case SDLK_F1: return PXL_KEYB_F1;
        case SDLK_F2: return PXL_KEYB_F2;
        case SDLK_F3: return PXL_KEYB_F3;
        case SDLK_F4: return PXL_KEYB_F4;
        case SDLK_F5: return PXL_KEYB_F5;
        case SDLK_F6: return PXL_KEYB_F6;
        case SDLK_F7: return PXL_KEYB_F7;
        case SDLK_F8: return PXL_KEYB_F8;
        case SDLK_F9: return PXL_KEYB_F9;
        case SDLK_F10: return PXL_KEYB_F10;
        case SDLK_F11: return PXL_KEYB_F11;
        case SDLK_F12: return PXL_KEYB_F12;
        case SDLK_F13: return PXL_KEYB_F13;
        case SDLK_F14: return PXL_KEYB_F14;
        case SDLK_F15: return PXL_KEYB_F15;
        case SDLK_F16: return PXL_KEYB_F16;
        case SDLK_F17: return PXL_KEYB_F17;
        case SDLK_F18: return PXL_KEYB_F18;
        case SDLK_F19: return PXL_KEYB_F19;
        case SDLK_F20: return PXL_KEYB_F20;
        case SDLK_F21: return PXL_KEYB_F21;
        case SDLK_F22: return PXL_KEYB_F22;
        case SDLK_F23: return PXL_KEYB_F23;
        case SDLK_F24: return PXL_KEYB_F24;

        case SDLK_KP_0: return PXL_KEYB_KP_0;
        case SDLK_KP_1: return PXL_KEYB_KP_1;
        case SDLK_KP_2: return PXL_KEYB_KP_2;
        case SDLK_KP_3: return PXL_KEYB_KP_3;
        case SDLK_KP_4: return PXL_KEYB_KP_4;
        case SDLK_KP_5: return PXL_KEYB_KP_5;
        case SDLK_KP_6: return PXL_KEYB_KP_6;
        case SDLK_KP_7: return PXL_KEYB_KP_7;
        case SDLK_KP_8: return PXL_KEYB_KP_8;
        case SDLK_KP_9: return PXL_KEYB_KP_9;
        case SDLK_KP_PERIOD: return PXL_KEYB_KP_DECIMAL;
        case SDLK_KP_DIVIDE: return PXL_KEYB_KP_DIVIDE;
        case SDLK_KP_MULTIPLY: return PXL_KEYB_KP_MULTIPLY;
        case SDLK_KP_MINUS: return PXL_KEYB_KP_SUBTRACT;
        case SDLK_KP_PLUS: return PXL_KEYB_KP_ADD;
        case SDLK_KP_ENTER: return PXL_KEYB_KP_ENTER;
        case SDLK_KP_EQUALS: return PXL_KEYB_KP_EQUAL;

        default: return PXL_IN_UNKNOWN;
    }
}

static pxl_input_code_t
sdl_button_to_pxl_input_code(const Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT:   return PXL_MOUSE_LEFT;
        case SDL_BUTTON_RIGHT:  return PXL_MOUSE_RIGHT;
        case SDL_BUTTON_MIDDLE: return PXL_MOUSE_MIDDLE;
        default: return PXL_IN_UNKNOWN;
    }
}

/* Process a single SDL event and update input state */
static void
process_sdl_event(SDL_Event *event, pxl_input_t *in) {
    switch (event->type) {
        case SDL_QUIT:
			pxl_input_press(in, PXL_WM_QUIT);
            break;

        case SDL_KEYDOWN:
            if (!event->key.repeat) {
				pxl_input_press(in, sdl_keysym_to_pxl_input_code(event->key.keysym.sym));
            }
            break;

        case SDL_KEYUP:
			pxl_input_release(in, sdl_keysym_to_pxl_input_code(event->key.keysym.sym));
            break;

        case SDL_MOUSEBUTTONDOWN:
			pxl_input_press(in, sdl_button_to_pxl_input_code(event->button.button));
            break;

        case SDL_MOUSEBUTTONUP:
			pxl_input_release(in, sdl_button_to_pxl_input_code(event->button.button));
            break;

        case SDL_MOUSEMOTION:
            in->mouse_x = event->motion.x;
            in->mouse_y = event->motion.y;
            break;

        case SDL_MOUSEWHEEL:
            in->mouse_wheel_x += event->wheel.x;
            in->mouse_wheel_y += event->wheel.y;
            break;

        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_ENTER) {
                pxl_input_release(in, PXL_WM_MOUSE_FOCUS_LOST);
            } else if (event->window.event == SDL_WINDOWEVENT_LEAVE) {
                pxl_input_press(in, PXL_WM_MOUSE_FOCUS_LOST);
            } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                pxl_input_release(in, PXL_WM_FOCUS_LOST);
            } else if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                pxl_input_press(in, PXL_WM_FOCUS_LOST);
            }
            break;

        case SDL_TEXTINPUT:
            /* FIFO: Append UTF-8 text to internal buffer (no null-termination) */
            {
                int len = strlen(event->text.text);
                if (g_sdl.text_buffer_len + len > (int)sizeof(g_sdl.text_buffer)) {
                    int excess = (g_sdl.text_buffer_len + len) - (int)sizeof(g_sdl.text_buffer);
                    memmove(g_sdl.text_buffer, g_sdl.text_buffer + excess, g_sdl.text_buffer_len - excess);
                    g_sdl.text_buffer_len -= excess;
                }
                memcpy(g_sdl.text_buffer + g_sdl.text_buffer_len, event->text.text, len);
                g_sdl.text_buffer_len += len;
            }
            break;
    }
}

void
pxl_backend_poll_events(pxl_input_t *in) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        process_sdl_event(&event, in);
    }
}

void
pxl_backend_wait_events(pxl_input_t *in) {
    SDL_Event event;
    if (SDL_WaitEvent(&event)) {
        process_sdl_event(&event, in);
        while (SDL_PollEvent(&event)) {
            process_sdl_event(&event, in);
        }
    }
}

int
pxl_backend_get_typed_text(char *out_text, int out_text_max_len) {
    assert(out_text);
    assert(out_text_max_len > 0);

    if (g_sdl.text_buffer_len == 0) return 0;

    int copy_len = (g_sdl.text_buffer_len < out_text_max_len)
        ? g_sdl.text_buffer_len
        : out_text_max_len - 1;

    /* Early return if nothing to copy (kept for clarity and to avoid useless operations) */
    if (copy_len <= 0) return 0;

    memcpy(out_text, g_sdl.text_buffer, copy_len);
    out_text[copy_len] = '\0';

    /* Consume copied bytes (no null-termination in internal buffer) */
    g_sdl.text_buffer_len -= copy_len;
    memmove(g_sdl.text_buffer, g_sdl.text_buffer + copy_len, g_sdl.text_buffer_len);

    return copy_len;
}
