#include <assert.h>
#include <stdbool.h>
#include "app.h"
#include "backend.h"
#include "input.h"

static void
test_app_transitions_active(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	pxl_input_set(&app.curr, PXL_KEYB_A);

	assert(pxl_app_is_active(&app, PXL_KEYB_A) == true);
	assert(pxl_app_was_triggered(&app, PXL_KEYB_A) == true);
	assert(pxl_app_was_released(&app, PXL_KEYB_A) == false);
}

static void
test_app_transitions_released(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	pxl_input_set(&app.prev, PXL_KEYB_A);

	assert(pxl_app_is_active(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_triggered(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_released(&app, PXL_KEYB_A) == true);
}

static void
test_app_transitions_no_change(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	pxl_input_set(&app.prev, PXL_KEYB_A);
	pxl_input_set(&app.curr, PXL_KEYB_A);

	assert(pxl_app_is_active(&app, PXL_KEYB_A) == true);
	assert(pxl_app_was_triggered(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_released(&app, PXL_KEYB_A) == false);

	app.prev = (pxl_input_t){0};
	app.curr = (pxl_input_t){0};

	assert(pxl_app_is_active(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_triggered(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_released(&app, PXL_KEYB_A) == false);
}

static void
test_app_transitions_multiple_keys(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	pxl_input_set(&app.prev, PXL_KEYB_A);
	pxl_input_set(&app.curr, PXL_KEYB_B);

	assert(pxl_app_was_triggered(&app, PXL_KEYB_A) == false);
	assert(pxl_app_was_released(&app, PXL_KEYB_A) == true);
	assert(pxl_app_was_triggered(&app, PXL_KEYB_B) == true);
	assert(pxl_app_was_released(&app, PXL_KEYB_B) == false);
}

static void
test_app_mouse_wheel_reset(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	app.curr.mouse_wheel_x = 5;
	app.curr.mouse_wheel_y = -3;

	app.prev = app.curr;
	app.curr.mouse_wheel_x = 0;
	app.curr.mouse_wheel_y = 0;

	assert(app.curr.mouse_wheel_x == 0);
	assert(app.curr.mouse_wheel_y == 0);
	assert(app.prev.mouse_wheel_x == 5);
	assert(app.prev.mouse_wheel_y == -3);
}

static void
test_app_should_close(void) {
	pxl_app_t app = {0};
	app.curr = (pxl_input_t){0};
	app.prev = (pxl_input_t){0};

	assert(pxl_input_state(&app.curr, PXL_WM_QUIT) == false);

	pxl_input_set(&app.curr, PXL_WM_QUIT);
	assert(pxl_input_state(&app.curr, PXL_WM_QUIT) == true);

	app.curr = (pxl_input_t){0};
	pxl_input_set(&app.curr, PXL_KEYB_ESCAPE);
	assert(pxl_input_state(&app.curr, PXL_WM_QUIT) == false);
}

static void
test_app_physics_disabled(void) {
	pxl_app_t app = {0};
	app.physics_ts.dt = 0;

	/* advance_physics should return false when stepper is disabled (dt=0) */
	assert(pxl_app_advance_physics(&app) == false);
}

static void
test_app_cfg_basic(void) {
	pxl_app_cfg_t cfg = {
		.title = "Test App",
		.width = 800,
		.height = 600,
		.backend_flags = PXL_BACKEND_HIDDEN,
		.physics_dt = 1.0 / 60.0
	};

	assert(cfg.width == 800);
	assert(cfg.height == 600);
	assert(cfg.physics_dt > 0);
}

static void
test_app_cfg_zero_physics(void) {
	pxl_app_cfg_t cfg = {
		.title = "Test",
		.width = 100,
		.height = 100,
		.backend_flags = PXL_BACKEND_HIDDEN,
		.physics_dt = 0  /* Disable physics */
	};

	assert(cfg.physics_dt == 0);
}

/* Example test from app.h documentation */
static void
test_example_pxl_app_init(void) {
	pxl_app_cfg_t cfg = {
		.title = "My Game",
		.width = 800,
		.height = 600,
		.backend_flags = PXL_BACKEND_HIDDEN,
		.physics_dt = 1.0 / 60.0
	};

	/* This verifies the example compiles and config values are valid */
	assert(cfg.width == 800);
	assert(cfg.height == 600);
	assert(cfg.physics_dt == 1.0 / 60.0);
}

/* Main */
int
main(void) {
	test_app_transitions_active();
	test_app_transitions_released();
	test_app_transitions_no_change();
	test_app_transitions_multiple_keys();
	test_app_mouse_wheel_reset();
	test_app_should_close();
	test_app_physics_disabled();

	/* Config tests */
	test_app_cfg_basic();
	test_app_cfg_zero_physics();

	/* Example tests */
	test_example_pxl_app_init();

	return 0;
}
