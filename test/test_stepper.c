#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include "stepper.h"

/* Initialization tests */

static void
test_pxl_stepper_init(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.016);

    assert(ts.accumulator == 0.0);
    assert(ts.alpha == 0.0f);
    assert(ts.dt == 0.016);
}

/* Update tests */

static void
test_pxl_stepper_update(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.016);

    pxl_stepper_update(&ts, 0.017);
    assert(ts.accumulator == 0.017);
}

/* Advance tests */

static void
test_pxl_stepper_advance_trigger(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.016);

    pxl_stepper_update(&ts, 0.017);
    assert(pxl_stepper_advance(&ts) == true);
    assert(fabs(ts.accumulator - 0.001) < 0.0001); /* accumulator = 0.017 - 0.016 = 0.001 */
}

static void
test_pxl_stepper_advance_no_trigger(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.016);

    pxl_stepper_update(&ts, 0.009);
    assert(pxl_stepper_advance(&ts) == false);
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

    assert(steps == 3);
    assert(fabs(ts.accumulator - 0.005) < 0.0001);
}

static void
test_pxl_stepper_alpha(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.016);

    pxl_stepper_update(&ts, 0.008);
    assert(pxl_stepper_advance(&ts) == false);
    assert(ts.alpha == 0.5f); /* 0.008 / 0.016 = 0.5 */
}

static void
test_pxl_stepper_disabled(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.0);

    pxl_stepper_update(&ts, 0.017);
    assert(ts.accumulator == 0.0); /* dt=0 means disabled, accumulator should not change */
    assert(pxl_stepper_advance(&ts) == false);
}

static void
test_pxl_stepper_zero_dt_init(void) {
    pxl_time_stepper_t ts;
    pxl_stepper_init(&ts, 0.0);

    assert(ts.dt == 0.0);
    assert(ts.accumulator == 0.0);
    assert(ts.alpha == 0.0f);
}

/* Main */

int
main(void) {
    test_pxl_stepper_init();
    test_pxl_stepper_update();
    test_pxl_stepper_advance_trigger();
    test_pxl_stepper_advance_no_trigger();
    test_pxl_stepper_advance_multiple_steps();
    test_pxl_stepper_alpha();
    test_pxl_stepper_disabled();
    test_pxl_stepper_zero_dt_init();
    return 0;
}
