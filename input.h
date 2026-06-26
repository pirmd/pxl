#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <string.h>

typedef enum {
    KEY_UNKNOWN = 0,

    // Common controls
    KEY_ESCAPE,
    KEY_ENTER,
    KEY_TAB,
    KEY_BACKSPACE,
    KEY_INSERT,
    KEY_DELETE,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,

    // Arrows
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    // Modifiers
    KEY_LSHIFT, KEY_RSHIFT,
    KEY_LCTRL,  KEY_RCTRL,
    KEY_LALT,   KEY_RALT,
    KEY_LSUPER, KEY_RSUPER,

    // Punctuation / symbols
    KEY_SPACE,
    KEY_APOSTROPHE,
    KEY_COMMA,
    KEY_MINUS,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_SEMICOLON,
    KEY_EQUAL,
    KEY_LEFT_BRACKET,
    KEY_BACKSLASH,
    KEY_RIGHT_BRACKET,
    KEY_GRAVE_ACCENT,

    // Number row
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // Letters
    KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
    KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
    KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

    // Function keys
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18, KEY_F19, KEY_F20,
    KEY_F21, KEY_F22, KEY_F23, KEY_F24,

    // Numpad
    KEY_KP_0, KEY_KP_1, KEY_KP_2, KEY_KP_3, KEY_KP_4,
    KEY_KP_5, KEY_KP_6, KEY_KP_7, KEY_KP_8, KEY_KP_9,
    KEY_KP_DECIMAL,
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT,
    KEY_KP_ADD,
    KEY_KP_ENTER,
    KEY_KP_EQUAL,

    KEY_COUNT
} key_code_t;

typedef enum {
	MOUSE_UNKNOWN = 0,

    MOUSE_LEFT,
    MOUSE_RIGHT,
    MOUSE_MIDDLE,

    MOUSE_COUNT
} mouse_button_t;

typedef struct {
	bool keys[KEY_COUNT];
	bool keys_prev[KEY_COUNT];

    bool mouse_buttons[MOUSE_COUNT];
    bool mouse_buttons_prev[MOUSE_COUNT];

    int mouse_x, mouse_y;
    int mouse_dx, mouse_dy;
    int mouse_wheel_x, mouse_wheel_y;

    bool quit_requested;
} input_t;

static inline void
input_begin_frame(input_t *in) {
	assert(sizeof(in->keys) == sizeof(in->keys_prev));
	memcpy(in->keys_prev, in->keys, sizeof(in->keys));

	assert(sizeof(in->mouse_buttons) == sizeof(in->mouse_buttons_prev));
	memcpy(in->mouse_buttons_prev, in->mouse_buttons, sizeof(in->mouse_buttons));

    in->mouse_dx = 0;
    in->mouse_dy = 0;

    in->mouse_wheel_x = 0;
    in->mouse_wheel_y = 0;

    in->quit_requested = false;
}

static inline void
input_set_mouse_pos(input_t *in, int x, int y) {
    in->mouse_dx += x - in->mouse_x;
    in->mouse_dy += y - in->mouse_y;

    in->mouse_x = x;
    in->mouse_y = y;
}

static inline void
input_add_mouse_wheel(input_t *in, int dx, int dy) {
    in->mouse_wheel_x += dx;
    in->mouse_wheel_y += dy;
}

static inline bool
input_key_pressed(const input_t *in, key_code_t key) {
	assert(key > KEY_UNKNOWN && key < KEY_COUNT);
	return in->keys[key] && !in->keys_prev[key];
}

static inline bool
input_key_released(const input_t *in, key_code_t key) {
	assert(key > KEY_UNKNOWN && key < KEY_COUNT);
	return !in->keys[key] && in->keys_prev[key];
}

static inline bool
input_mouse_pressed(const input_t *in, mouse_button_t button) {
	assert(button > MOUSE_UNKNOWN && button < MOUSE_COUNT);
	return in->mouse_buttons[button] && !in->mouse_buttons_prev[button];
}

static inline bool
input_mouse_released(const input_t *in, mouse_button_t button) {
	assert(button > MOUSE_UNKNOWN && button < MOUSE_COUNT);
	return !in->mouse_buttons[button] && in->mouse_buttons_prev[button];
}

static inline bool
input_shift_down(const input_t *in) {
    return in->keys[KEY_LSHIFT] || in->keys[KEY_RSHIFT];
}

static inline bool
input_ctrl_down(const input_t *in) {
    return in->keys[KEY_LCTRL] || in->keys[KEY_RCTRL];
}

static inline bool
input_alt_down(const input_t *in) {
    return in->keys[KEY_LALT] || in->keys[KEY_RALT];
}

static inline bool
input_super_down(const input_t *in) {
    return in->keys[KEY_LSUPER] || in->keys[KEY_RSUPER];
}

#endif /* INPUT_H */
