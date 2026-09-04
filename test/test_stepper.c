#include "stepper.h"
#include "test.h"
#include <math.h>

/* Initialization tests */

static void
test_pxl_stepper_init(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	ASSERT(ts.accumulator == 0.0);
	ASSERT(ts.alpha == 0.0f);
	ASSERT(ts.dt == 0.016);
}

/* Update tests */

static void
test_pxl_stepper_update_basic(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	pxl_stepper_update(&ts, 0.017);
	ASSERT(ts.accumulator == 0.017);
}

static void
test_pxl_stepper_update_accumulate(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	pxl_stepper_update(&ts, 0.017);
	ASSERT(ts.accumulator == 0.017);
}

/* Advance tests */

static void
test_pxl_stepper_advance_trigger(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	pxl_stepper_update(&ts, 0.017);
	ASSERT(pxl_stepper_advance(&ts) == true);
	ASSERT(fabs(ts.accumulator - 0.001) < 0.0001); /* accumulator = 0.017 - 0.016 = 0.001 */
}

static void
test_pxl_stepper_advance_no_trigger(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	pxl_stepper_update(&ts, 0.009);
	ASSERT(pxl_stepper_advance(&ts) == false);
}

static void
test_pxl_stepper_advance_multiple_steps(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.01);

	pxl_stepper_update(&ts, 0.035);
	
	int steps = 0;
	while (pxl_stepper_advance(&ts)) {
		steps++;
	}
	
	ASSERT(steps == 3);
	ASSERT(fabs(ts.accumulator - 0.005) < 0.0001);
}

static void
test_pxl_stepper_alpha(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.016);

	pxl_stepper_update(&ts, 0.008);
	ASSERT(pxl_stepper_advance(&ts) == false);
	ASSERT(ts.alpha == 0.5f); /* 0.008 / 0.016 = 0.5 */
}

static void
test_pxl_stepper_disabled(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.0);

	pxl_stepper_update(&ts, 0.017);
	ASSERT(ts.accumulator == 0.0); /* dt=0 means disabled, accumulator should not change */
	ASSERT(pxl_stepper_advance(&ts) == false);
}

static void
test_pxl_stepper_zero_dt_init(void) {
	pxl_time_stepper_t ts;
	pxl_stepper_init(&ts, 0.0);

	ASSERT(ts.dt == 0.0);
	ASSERT(ts.accumulator == 0.0);
	ASSERT(ts.alpha == 0.0f);
}

/* Main */

int
main(void) {
	test_pxl_stepper_init();
	test_pxl_stepper_update_basic();
	test_pxl_stepper_update_accumulate();
	test_pxl_stepper_advance_trigger();
	test_pxl_stepper_advance_no_trigger();
	test_pxl_stepper_advance_multiple_steps();
	test_pxl_stepper_alpha();
	test_pxl_stepper_disabled();
	test_pxl_stepper_zero_dt_init();
	return 0;
}
