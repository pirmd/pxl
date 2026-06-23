#include "backend.h"
#include <SDL.h>

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

	assert(pitch % sizeof(pix_t) == 0);

	out_pb->width = g_sdl.width;
	out_pb->height = g_sdl.height;
	out_pb->stride = pitch / sizeof(pix_t);
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

static int
sdl_keysym_to_key(SDL_Keycode keysym) {
	switch (keysym) {
		case SDLK_UP:     return KEY_UP;
		case SDLK_DOWN:   return KEY_DOWN;
		case SDLK_LEFT:   return KEY_LEFT;
		case SDLK_RIGHT:  return KEY_RIGHT;
		case SDLK_SPACE:  return KEY_SPACE;
		case SDLK_ESCAPE: return KEY_ESCAPE;
		default: return -1;
	}
}

bool
backend_poll_events(input_t *input) {
	SDL_Event event;

	input_save_prev_state(input);

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				input->quit_requested = true;
				return false;

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				bool pressed = (event.type == SDL_KEYDOWN);
				int key = sdl_keysym_to_key(event.key.keysym.sym);
				if (key >= 0 && key < KEY_COUNT) {
					input->keys[key] = pressed;
				}
				break;
			}

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				bool pressed = (event.type == SDL_MOUSEBUTTONDOWN);
				if (event.button.button == SDL_BUTTON_LEFT) {
					input->mouse_left = pressed;
				}
				if (event.button.button == SDL_BUTTON_RIGHT) {
					input->mouse_right = pressed;
				}
				break;
			}

			case SDL_MOUSEMOTION:
				input->mouse_x = event.motion.x;
				input->mouse_y = event.motion.y;
				break;
		}
	}
	return true;
}
