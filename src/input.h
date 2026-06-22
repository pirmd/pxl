#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

typedef enum {
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_SPACE, KEY_ESCAPE,
	KEY_COUNT
} key_code_t;

typedef struct {
	bool keys[KEY_COUNT];
	bool keys_prev[KEY_COUNT];
	
	int  mouse_x, mouse_y;
	bool mouse_left;
	bool mouse_left_prev;
	bool mouse_right;
	bool mouse_right_prev;

	bool quit_requested;
} input_t;

static inline void
input_save_prev_state(input_t *in) {
	for (int i = 0; i < KEY_COUNT; i++) {
		in->keys_prev[i] = in->keys[i];
	}
	in->mouse_left_prev = in->mouse_left;
	in->mouse_right_prev = in->mouse_right;
}

static inline bool
input_is_down(const input_t *in, key_code_t key) {
	return in->keys[key];
}

static inline bool
input_is_pressed(const input_t *in, key_code_t key) {
	return in->keys[key] && !in->keys_prev[key];
}

static inline bool
input_mouse_pressed(const input_t *in, bool left) {
	return left ? (in->mouse_left && !in->mouse_left_prev)
	            : (in->mouse_right && !in->mouse_right_prev);
}


#endif /* INPUT_H */
