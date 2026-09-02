#ifndef PXL_INPUT_H
#define PXL_INPUT_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Gather input state: physical keys, mouse button or windows-related events
 *
 * Physical key codes represent physical keys and not characters: names
 * correspond to QWERTY keyboard layout positions.
 *
 */
typedef enum {
    PXL_IN_UNKNOWN = 0,

    /* Common controls */
    PXL_KEYB_ESCAPE,
    PXL_KEYB_ENTER,
    PXL_KEYB_TAB,
    PXL_KEYB_BACKSPACE,
    PXL_KEYB_INSERT,
    PXL_KEYB_DELETE,
    PXL_KEYB_HOME,
    PXL_KEYB_END,
    PXL_KEYB_PAGE_UP,
    PXL_KEYB_PAGE_DOWN,

    /* Arrows */
    PXL_KEYB_UP,
    PXL_KEYB_DOWN,
    PXL_KEYB_LEFT,
    PXL_KEYB_RIGHT,

    /* Modifiers */
    PXL_KEYB_LSHIFT, PXL_KEYB_RSHIFT,
    PXL_KEYB_LCTRL,  PXL_KEYB_RCTRL,
    PXL_KEYB_LALT,   PXL_KEYB_RALT,
    PXL_KEYB_LSUPER, PXL_KEYB_RSUPER,

    /* Punctuation / symbols */
    PXL_KEYB_SPACE,
    PXL_KEYB_APOSTROPHE,
    PXL_KEYB_COMMA,
    PXL_KEYB_MINUS,
    PXL_KEYB_PERIOD,
    PXL_KEYB_SLASH,
    PXL_KEYB_SEMICOLON,
    PXL_KEYB_EQUAL,
    PXL_KEYB_LEFT_BRACKET,
    PXL_KEYB_BACKSLASH,
    PXL_KEYB_RIGHT_BRACKET,
    PXL_KEYB_GRAVE_ACCENT,

    /* Number row */
    PXL_KEYB_0, PXL_KEYB_1, PXL_KEYB_2, PXL_KEYB_3, PXL_KEYB_4,
    PXL_KEYB_5, PXL_KEYB_6, PXL_KEYB_7, PXL_KEYB_8, PXL_KEYB_9,

    /* Letters */
    PXL_KEYB_A, PXL_KEYB_B, PXL_KEYB_C, PXL_KEYB_D, PXL_KEYB_E, PXL_KEYB_F, PXL_KEYB_G, PXL_KEYB_H, PXL_KEYB_I, PXL_KEYB_J,
    PXL_KEYB_K, PXL_KEYB_L, PXL_KEYB_M, PXL_KEYB_N, PXL_KEYB_O, PXL_KEYB_P, PXL_KEYB_Q, PXL_KEYB_R, PXL_KEYB_S, PXL_KEYB_T,
    PXL_KEYB_U, PXL_KEYB_V, PXL_KEYB_W, PXL_KEYB_X, PXL_KEYB_Y, PXL_KEYB_Z,

    /* Function keys */
    PXL_KEYB_F1, PXL_KEYB_F2, PXL_KEYB_F3, PXL_KEYB_F4, PXL_KEYB_F5, PXL_KEYB_F6,
    PXL_KEYB_F7, PXL_KEYB_F8, PXL_KEYB_F9, PXL_KEYB_F10, PXL_KEYB_F11, PXL_KEYB_F12,
    PXL_KEYB_F13, PXL_KEYB_F14, PXL_KEYB_F15, PXL_KEYB_F16, PXL_KEYB_F17, PXL_KEYB_F18, PXL_KEYB_F19, PXL_KEYB_F20,
    PXL_KEYB_F21, PXL_KEYB_F22, PXL_KEYB_F23, PXL_KEYB_F24,

    /* Numpad */
    PXL_KEYB_KP_0, PXL_KEYB_KP_1, PXL_KEYB_KP_2, PXL_KEYB_KP_3, PXL_KEYB_KP_4,
    PXL_KEYB_KP_5, PXL_KEYB_KP_6, PXL_KEYB_KP_7, PXL_KEYB_KP_8, PXL_KEYB_KP_9,
    PXL_KEYB_KP_DECIMAL,
    PXL_KEYB_KP_DIVIDE,
    PXL_KEYB_KP_MULTIPLY,
    PXL_KEYB_KP_SUBTRACT,
    PXL_KEYB_KP_ADD,
    PXL_KEYB_KP_ENTER,
    PXL_KEYB_KP_EQUAL,

	/* Mouse buttons */
    PXL_MOUSE_LEFT,
    PXL_MOUSE_RIGHT,
    PXL_MOUSE_MIDDLE,

	/* WM events */
	PXL_WM_FOCUS_LOST,
	PXL_WM_MOUSE_FOCUS_LOST,
	PXL_WM_QUIT,

    PXL_IN_COUNT
} pxl_input_code_t;

/*
 * pxl_input_t: Gathers current input state (keyboard, mouse, WM events).
 * MUST be zero-initialized before first use (e.g., pxl_input_t in = {0};).
 * Backends only UPDATE its fields via pxl_backend_poll_events().
 */
typedef struct {
	uint64_t state[(PXL_IN_COUNT + 63) / 64];    /* Current key/mouse button state */
	int mouse_x, mouse_y;                        /* Current mouse position         */
	int mouse_wheel_x, mouse_wheel_y;            /* Wheel delta since last reset   */
} pxl_input_t;

/* Check key/mouse button state. Returns true if pressed/down, false if released/up. */
static inline bool
pxl_input_state(const pxl_input_t *in, pxl_input_code_t c) {
	assert(in);
	assert(c >= 0 && c < PXL_IN_COUNT);
	return (in->state[c / 64] & (1ULL << (c % 64))) != 0;
}


/*
 *  Mostly for backends internal use
 */

/* Set key/mouse button as pressed in input state bitset */
static inline void
pxl_input_press(pxl_input_t *in, pxl_input_code_t code) {
	assert(in && code >= 0 && code < PXL_IN_COUNT);
	in->state[code / 64] |= (1ULL << (code % 64));
}

/* Set key/mouse button as released in input state bitset */
static inline void
pxl_input_release(pxl_input_t *in, pxl_input_code_t code) {
	assert(in && code >= 0 && code < PXL_IN_COUNT);
	in->state[code / 64] &= ~(1ULL << (code % 64));
}

#endif /* PXL_INPUT_H */
