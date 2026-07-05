#include "input.h"
#include "stest/stest.h"

/* State tests --------------------------------------------------------------- */

static void
test_input_init(void) {
	pxl_input_t input;
	pxl_input_init(&input);

	ST_CHECK(input.cur.mouse_x == 0, "mouse.x not initialized to 0");
	ST_CHECK(input.cur.mouse_y == 0, "mouse.y not initialized to 0");
	ST_CHECK(input.cur.mouse_wheel_x == 0, "mouse_wheel_x not initialized to 0");
	ST_CHECK(input.cur.mouse_wheel_y == 0, "mouse_wheel_y not initialized to 0");

	/* Verify all pressed bits are initialized to 0 */
	for (int i = 0; i < PXL_IN_BITSET_WORDS; i++) {
		ST_CHECK(input.cur.pressed[i] == 0, "pressed[%d] not initialized to 0", i);
	}
}

static void
test_input_press_release(void) {
	pxl_input_t input;
	pxl_input_init(&input);

	pxl_input_press(&input.cur, PXL_KEYB_A);
	ST_CHECK(pxl_input_pressed(&input.cur, PXL_KEYB_A), "PXL_KEYB_A should be pressed");
	ST_CHECK(!pxl_input_pressed(&input.cur, PXL_KEYB_B), "PXL_KEYB_B should not be pressed");

	pxl_input_release(&input.cur, PXL_KEYB_A);
	ST_CHECK(!pxl_input_pressed(&input.cur, PXL_KEYB_A), "PXL_KEYB_A should be released");
}

/* State transition tests -------------------------------------------------- */

static void
test_input_next_state(void) {
	pxl_input_t input;
	pxl_input_init(&input);

	pxl_input_press(&input.cur, PXL_KEYB_A);
	pxl_input_next_state(&input);

	ST_CHECK(pxl_input_pressed(&input.prev, PXL_KEYB_A), "PXL_KEYB_A should be in prev state");
}

/* Main ----------------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		ST_T(test_input_init),
		ST_T(test_input_press_release),
		ST_T(test_input_next_state)
	);
}
