#include "stepper.h"
#include "test.h"

/* Initialization tests */

static void
test_pxl_stepper_init(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.001);

	ASSERT(ts.current_time == 0.001);
	ASSERT(ts.accumulator == 0.0);
	ASSERT(ts.lerp_factor == 0.0);
	ASSERT(ts.paused == false);
	ASSERT(ts.time_scale == 1.0f);
}

/* Sync time tests */

static void
test_pxl_stepper_sync_time_basic(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.001);

	pxl_stepper_sync_time(&ts, 0.017);
	ASSERT(ts.current_time == 0.017);
	ASSERT(ts.accumulator == 0.016);
}

static void
test_pxl_stepper_sync_time_clamp(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.0);

	pxl_stepper_sync_time(&ts, 10.0);
	ASSERT(ts.current_time == 10.0);
	ASSERT(ts.accumulator == PXL_STEPPER_MAX_FRAME_TIME);
}

/* Advance tests */

static void
test_pxl_stepper_advance_trigger(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.001);

	pxl_stepper_sync_time(&ts, 0.017);
	ASSERT(pxl_stepper_advance(&ts) == true);
	ASSERT(ts.accumulator == 0.0);
}

static void
test_pxl_stepper_advance_no_trigger(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.001);

	pxl_stepper_sync_time(&ts, 0.009);
	ASSERT(pxl_stepper_advance(&ts) == false);
}

static void
test_pxl_stepper_lerp_factor(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.001);

	pxl_stepper_sync_time(&ts, 0.009);
	pxl_stepper_advance(&ts);
	ASSERT(ts.lerp_factor == 0.5f);
}

/* Paused tests */

static void
test_pxl_stepper_paused(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.0);
	ts.paused = true;

	pxl_stepper_sync_time(&ts, 0.1);
	ASSERT(ts.current_time == 0.1);
	ASSERT(ts.accumulator == 0.0);

	ASSERT(pxl_stepper_advance(&ts) == false);
	ASSERT(ts.accumulator == 0.0);
}

/* Time scale tests */

static void
test_pxl_stepper_time_scale(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.0);
	ts.time_scale = 2.0f;

	pxl_stepper_sync_time(&ts, 0.01);
	ASSERT(ts.accumulator == 0.02);

	pxl_stepper_init(&ts, 0.0);
	ts.time_scale = 0.5f;
	pxl_stepper_sync_time(&ts, 0.04);
	ASSERT(ts.accumulator == 0.02);
}

/* Reinit test */

static void
test_pxl_stepper_reinit(void) {
	pxl_time_stepper_t ts = {.dt = 0.016};
	pxl_stepper_init(&ts, 0.1);

	ts.current_time = 100.0;
	ts.accumulator = 50.0;
	ts.lerp_factor = 0.75f;
	ts.paused = true;
	ts.time_scale = 2.0f;

	pxl_stepper_reinit(&ts, 0.2);
	ASSERT(ts.current_time == 0.2);
	ASSERT(ts.accumulator == 0.0);
	ASSERT(ts.lerp_factor == 0.0);
	ASSERT(ts.paused == false);
	ASSERT(ts.time_scale == 1.0f);
}

/* Main */

int
main(void) {
	test_pxl_stepper_init();
	test_pxl_stepper_sync_time_basic();
	test_pxl_stepper_sync_time_clamp();
	test_pxl_stepper_advance_trigger();
	test_pxl_stepper_advance_no_trigger();
	test_pxl_stepper_lerp_factor();
	test_pxl_stepper_paused();
	test_pxl_stepper_time_scale();
	test_pxl_stepper_reinit();
	return 0;
}
