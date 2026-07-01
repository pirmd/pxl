#include "stepper.h"
#include "stest/stest.h"

static void
test_stepper_init(void) {
	time_stepper_t ts = {.dt = 0.016};
	stepper_init(&ts, 0.001);

	ST_CHECK(ts.current_time == 0.001, "current_time not initialized");
	ST_CHECK(ts.accumulator == 0.0, "accumulator not initialized to 0");
	ST_CHECK(ts.lerp_factor == 0.0, "lerp_factor not initialized to 0");
}

static void
test_stepper_sync_time_basic(void) {
	time_stepper_t ts = {.dt = 0.016};
	stepper_init(&ts, 0.001);

	stepper_sync_time(&ts, 0.017);
	ST_CHECK(ts.current_time == 0.017, "current_time not updated");
	ST_CHECK(ts.accumulator == 0.016, "accumulator not updated");
}

static void
test_stepper_advance_trigger(void) {
	time_stepper_t ts = {.dt = 0.016};
	stepper_init(&ts, 0.001);

	stepper_sync_time(&ts, 0.017);
	ST_CHECK(stepper_advance(&ts) == true, "should advance (accumulator >= dt)");
	ST_CHECK(ts.accumulator == 0.0, "accumulator not reset after advance");
}

static void
test_stepper_advance_no_trigger(void) {
	time_stepper_t ts = {.dt = 0.016};
	stepper_init(&ts, 0.001);

	stepper_sync_time(&ts, 0.009);
	ST_CHECK(stepper_advance(&ts) == false, "should not advance (accumulator < dt)");
}

static void
test_stepper_lerp_factor(void) {
	time_stepper_t ts = {.dt = 0.016};
	stepper_init(&ts, 0.001);

	stepper_sync_time(&ts, 0.009);
	stepper_advance(&ts);
	ST_CHECK(ts.lerp_factor == 0.5f, "lerp_factor should be 0.5 (8ms/16ms)");
}

/* Main */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		ST_T(test_stepper_init),
		ST_T(test_stepper_sync_time_basic),
		ST_T(test_stepper_advance_trigger),
		ST_T(test_stepper_advance_no_trigger),
		ST_T(test_stepper_lerp_factor)
	);
}
