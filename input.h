#ifndef INPUT_H
#define INPUT_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define IN_BITSET_MAX 128
#define IN_BITSET_WORDS (IN_BITSET_MAX / 64)

typedef enum {
    IN_UNKNOWN = 0,

    // Common controls
    IN_KEYB_ESCAPE,
    IN_KEYB_ENTER,
    IN_KEYB_TAB,
    IN_KEYB_BACKSPACE,
    IN_KEYB_INSERT,
    IN_KEYB_DELETE,
    IN_KEYB_HOME,
    IN_KEYB_END,
    IN_KEYB_PAGE_UP,
    IN_KEYB_PAGE_DOWN,

    // Arrows
    IN_KEYB_UP,
    IN_KEYB_DOWN,
    IN_KEYB_LEFT,
    IN_KEYB_RIGHT,

    // Modifiers
    IN_KEYB_LSHIFT, IN_KEYB_RSHIFT,
    IN_KEYB_LCTRL,  IN_KEYB_RCTRL,
    IN_KEYB_LALT,   IN_KEYB_RALT,
    IN_KEYB_LSUPER, IN_KEYB_RSUPER,

    // Punctuation / symbols
    IN_KEYB_SPACE,
    IN_KEYB_APOSTROPHE,
    IN_KEYB_COMMA,
    IN_KEYB_MINUS,
    IN_KEYB_PERIOD,
    IN_KEYB_SLASH,
    IN_KEYB_SEMICOLON,
    IN_KEYB_EQUAL,
    IN_KEYB_LEFT_BRACKET,
    IN_KEYB_BACKSLASH,
    IN_KEYB_RIGHT_BRACKET,
    IN_KEYB_GRAVE_ACCENT,

    // Number row
    IN_KEYB_0, IN_KEYB_1, IN_KEYB_2, IN_KEYB_3, IN_KEYB_4,
    IN_KEYB_5, IN_KEYB_6, IN_KEYB_7, IN_KEYB_8, IN_KEYB_9,

    // Letters
    IN_KEYB_A, IN_KEYB_B, IN_KEYB_C, IN_KEYB_D, IN_KEYB_E, IN_KEYB_F, IN_KEYB_G, IN_KEYB_H, IN_KEYB_I, IN_KEYB_J,
    IN_KEYB_K, IN_KEYB_L, IN_KEYB_M, IN_KEYB_N, IN_KEYB_O, IN_KEYB_P, IN_KEYB_Q, IN_KEYB_R, IN_KEYB_S, IN_KEYB_T,
    IN_KEYB_U, IN_KEYB_V, IN_KEYB_W, IN_KEYB_X, IN_KEYB_Y, IN_KEYB_Z,

    // Function keys
    IN_KEYB_F1, IN_KEYB_F2, IN_KEYB_F3, IN_KEYB_F4, IN_KEYB_F5, IN_KEYB_F6,
    IN_KEYB_F7, IN_KEYB_F8, IN_KEYB_F9, IN_KEYB_F10, IN_KEYB_F11, IN_KEYB_F12,
    IN_KEYB_F13, IN_KEYB_F14, IN_KEYB_F15, IN_KEYB_F16, IN_KEYB_F17, IN_KEYB_F18, IN_KEYB_F19, IN_KEYB_F20,
    IN_KEYB_F21, IN_KEYB_F22, IN_KEYB_F23, IN_KEYB_F24,

    // Numpad
    IN_KEYB_KP_0, IN_KEYB_KP_1, IN_KEYB_KP_2, IN_KEYB_KP_3, IN_KEYB_KP_4,
    IN_KEYB_KP_5, IN_KEYB_KP_6, IN_KEYB_KP_7, IN_KEYB_KP_8, IN_KEYB_KP_9,
    IN_KEYB_KP_DECIMAL,
    IN_KEYB_KP_DIVIDE,
    IN_KEYB_KP_MULTIPLY,
    IN_KEYB_KP_SUBTRACT,
    IN_KEYB_KP_ADD,
    IN_KEYB_KP_ENTER,
    IN_KEYB_KP_EQUAL,

	// Mouse buttons
    IN_MOUSE_LEFT,
    IN_MOUSE_RIGHT,
    IN_MOUSE_MIDDLE,

	// WM events
	IN_WM_QUIT,

    IN_COUNT
} input_code_t;

/* Compile-time check: the number of input codes shall fit into the input state's bitset */
typedef char static_assert_input_count_fits_into_bitset[IN_COUNT <= IN_BITSET_MAX ? 1 : -1];


/* Input state ------------------------------------------------------------- */

typedef struct {
	uint64_t pressed[IN_BITSET_WORDS];

    int mouse_x, mouse_y;
    int mouse_wheel_x, mouse_wheel_y;
} input_state_t;

static inline void
input_init_state(input_state_t *state) {
    assert(state);
    *state = (input_state_t){0};
}

static inline void
input_reinit_state(input_state_t *in) {
    in->mouse_wheel_x = 0;
    in->mouse_wheel_y = 0;
}

static inline void
input_press(input_state_t *in, input_code_t c) {
	assert(in);
	assert(c >= IN_UNKNOWN && c < IN_COUNT);
	in->pressed[c / 64] |= (1ULL << (c % 64));
}

static inline void
input_release(input_state_t *in, input_code_t c) {
	assert(in);
	assert(c >= IN_UNKNOWN && c < IN_COUNT);
	in->pressed[c / 64] &= ~(1ULL << (c % 64));
}

static inline bool
input_pressed(const input_state_t *in, input_code_t c) {
	assert(in);
	assert(c >= IN_UNKNOWN && c < IN_COUNT);
	return (in->pressed[c / 64] & (1ULL << (c % 64))) != 0;
}

static inline void
input_set_mouse_pos(input_state_t *in, int x, int y) {
    assert(in);
	in->mouse_x = x;
    in->mouse_y = y;
}

static inline void
input_inc_mouse_wheel(input_state_t *in, int dx, int dy) {
    assert(in);
	in->mouse_wheel_x += dx;
    in->mouse_wheel_y += dy;
}

/* Input ------------------------------------------------------------------- */

typedef struct {
	input_state_t cur, prev;
} input_t;

static inline void
input_init(input_t *in) {
    assert(in);
    input_init_state(&in->cur);
    input_init_state(&in->prev);
}

static inline void
input_next_state(input_t *in) {
	assert(in);
	in->prev = in->cur;
	input_reinit_state(&in->cur);
}

static inline bool
input_is_pressed(const input_t *in, input_code_t c) {
	assert(in);
	return input_pressed(&in->cur, c);
}

static inline bool
input_was_pressed(const input_t *in, input_code_t c) {
	assert(in);
	return input_pressed(&in->cur, c) && !input_pressed(&in->prev, c);
}

static inline bool
input_was_released(const input_t *in, input_code_t c) {
	assert(in);
	return !input_pressed(&in->cur, c) && input_pressed(&in->prev, c);
}


#endif /* INPUT_H */
