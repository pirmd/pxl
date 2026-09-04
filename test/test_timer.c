#include "timer.h"
#include "test.h"
#include <math.h>

#define EPSILON 0.0001

/* Initialization tests */

static void
test_pxl_timer_start(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	ASSERT(timer.remaining == 2.0);
	ASSERT(timer.initial == 2.0);
}

static void
test_pxl_timer_finished(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	ASSERT(pxl_timer_finished(&timer) == false);

	/* Advance to completion */
	pxl_timer_advance(&timer, 2.0);
	ASSERT(pxl_timer_finished(&timer) == true);
}

static void
test_pxl_timer_advance_partial(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	/* Advance halfway */
	bool finished = pxl_timer_advance(&timer, 1.0);
	ASSERT(finished == false);
	ASSERT(timer.remaining == 1.0);
	ASSERT(timer.initial == 2.0);
}

static void
test_pxl_timer_advance_complete(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	/* Advance past completion */
	bool finished = pxl_timer_advance(&timer, 3.0);
	ASSERT(finished == true);
	ASSERT(timer.remaining <= 0.0);
}

static void
test_pxl_timer_advance_already_finished(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 1.0);
	
	/* Finish the timer */
	pxl_timer_advance(&timer, 1.0);
	ASSERT(pxl_timer_finished(&timer) == true);
	
	/* Advancing a finished timer does nothing */
	bool finished = pxl_timer_advance(&timer, 1.0);
	ASSERT(finished == false);
	ASSERT(timer.remaining <= 0.0);
}

static void
test_pxl_timer_progress(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	ASSERT(fabs(pxl_timer_progress(&timer) - 0.0) < EPSILON);

	pxl_timer_advance(&timer, 1.0);
	ASSERT(fabs(pxl_timer_progress(&timer) - 0.5) < EPSILON);

	pxl_timer_advance(&timer, 1.0);
	ASSERT(fabs(pxl_timer_progress(&timer) - 1.0) < EPSILON);
}

static void
test_pxl_timer_progress_finished(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	pxl_timer_advance(&timer, 3.0);
	/* Once finished, progress should be 1.0 */
	ASSERT(fabs(pxl_timer_progress(&timer) - 1.0) < EPSILON);
}

static void
test_pxl_timer_remaining(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);

	ASSERT(timer.remaining == 2.0);

	pxl_timer_advance(&timer, 0.5);
	ASSERT(timer.remaining == 1.5);

	pxl_timer_advance(&timer, 1.5);
	ASSERT(timer.remaining <= 0.0);
}

static void
test_pxl_timer_negative_remaining(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 2.0);
	
	/* Advance past completion */
	pxl_timer_advance(&timer, 3.0);
	ASSERT(timer.remaining < 0.0);
	ASSERT(pxl_timer_finished(&timer) == true);
	ASSERT(pxl_timer_progress(&timer) == 1.0);
}

static void
test_pxl_timer_small_advances(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 1.0);
	
	/* Many small advances */
	for (int i = 0; i < 10; i++) {
		pxl_timer_advance(&timer, 0.1);
	}
	
	/* Due to floating point precision, remaining might be very close to 0 */
	ASSERT(timer.remaining <= 0.0001);
	ASSERT(pxl_timer_finished(&timer) == true);
}

static void
test_pxl_timer_exact_completion(void) {
	pxl_timer_t timer;
	pxl_timer_start(&timer, 1.5);
	
	bool finished = pxl_timer_advance(&timer, 1.5);
	ASSERT(finished == true);
	ASSERT(fabs(timer.remaining - 0.0) < EPSILON);
}

/* Main */

int
main(void) {
	test_pxl_timer_start();
	test_pxl_timer_finished();
	test_pxl_timer_advance_partial();
	test_pxl_timer_advance_complete();
	test_pxl_timer_advance_already_finished();
	test_pxl_timer_progress();
	test_pxl_timer_progress_finished();
	test_pxl_timer_remaining();
	test_pxl_timer_negative_remaining();
	test_pxl_timer_small_advances();
	test_pxl_timer_exact_completion();
	return 0;
}
