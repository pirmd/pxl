#include "app.h"
#include "test.h"

static void
test_app_transitions_pressed(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;  /* Disable physics for unit tests */

	/* Previous frame: A was up, current frame: A is down */
	pxl_input_press(&app.curr, PXL_KEYB_A);

	ASSERT(pxl_app_is_pressed(&app, PXL_KEYB_A) == true);
	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_A) == true);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_A) == false);
}

static void
test_app_transitions_released(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;  /* Disable physics for unit tests */

	/* Previous frame: A was down */
	pxl_input_press(&app.prev, PXL_KEYB_A);
	/* Current frame: A is up */

	ASSERT(pxl_app_is_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_A) == true);
}

static void
test_app_transitions_no_change(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;  /* Disable physics for unit tests */

	/* A is down in both frames */
	pxl_input_press(&app.prev, PXL_KEYB_A);
	pxl_input_press(&app.curr, PXL_KEYB_A);

	ASSERT(pxl_app_is_pressed(&app, PXL_KEYB_A) == true);
	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_A) == false);

	/* A is up in both frames */
	app.prev = (pxl_input_t){0};
	app.curr = (pxl_input_t){0};

	ASSERT(pxl_app_is_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_A) == false);
}

static void
test_app_transitions_multiple_keys(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;  /* Disable physics for unit tests */

	/* Previous: A down, B up. Current: A up, B down */
	pxl_input_press(&app.prev, PXL_KEYB_A);
	pxl_input_press(&app.curr, PXL_KEYB_B);

	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_A) == false);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_A) == true);
	ASSERT(pxl_app_was_pressed(&app, PXL_KEYB_B) == true);
	ASSERT(pxl_app_was_released(&app, PXL_KEYB_B) == false);
}

static void
test_app_mouse_wheel_reset(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;  /* Disable physics for unit tests */

	/* Simulate wheel movement in current frame */
	app.curr.mouse_wheel_x = 5;
	app.curr.mouse_wheel_y = -3;

	/* After advance, wheel should be reset */
	app.prev = app.curr;
	app.curr.mouse_wheel_x = 0;
	app.curr.mouse_wheel_y = 0;

	ASSERT(app.curr.mouse_wheel_x == 0);
	ASSERT(app.curr.mouse_wheel_y == 0);
	ASSERT(app.prev.mouse_wheel_x == 5);
	ASSERT(app.prev.mouse_wheel_y == -3);
}

static void
test_app_should_close(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};
	app.physics_dt = 0;

	/* No quit condition */
	ASSERT(pxl_app_should_close(&app) == false);

	/* WM_QUIT pressed */
	pxl_input_press(&app.curr, PXL_WM_QUIT);
	ASSERT(pxl_app_should_close(&app) == true);

	/* Reset and test ESCAPE (should NOT trigger should_close) */
	app.curr = (pxl_input_t){0};
	pxl_input_press(&app.curr, PXL_KEYB_ESCAPE);
	ASSERT(pxl_app_should_close(&app) == false);
}

static void
test_app_physics_disabled(void) {
	pxl_app_t app = {0};
	app.physics_dt = 0;

	/* advance_physics should return false when physics disabled */
	ASSERT(pxl_app_advance_physics(&app) == false);
}

static void
test_app_physics_stepper_transparent(void) {
	pxl_app_t app = {0};
	app.physics_dt = 0;  /* Disabled */
	app.physics_ts.dt = 0;  /* Ensure stepper is disabled */

	/* advance_physics should return false when stepper is disabled */
	ASSERT(pxl_app_advance_physics(&app) == false);
}

/* Main */
int
main(void) {
	test_app_transitions_pressed();
	test_app_transitions_released();
	test_app_transitions_no_change();
	test_app_transitions_multiple_keys();
	test_app_mouse_wheel_reset();
	test_app_should_close();
	test_app_physics_disabled();
	test_app_physics_stepper_transparent();

	return 0;
}
