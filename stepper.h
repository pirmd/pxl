#ifndef PXL_STEPPER_H
#define PXL_STEPPER_H

#include <assert.h>
#include <stdbool.h>

typedef struct {
	double dt;
	float  lerp_factor;

	double current_time;
	double accumulator;
} pxl_time_stepper_t;

void
pxl_stepper_init(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->dt > 0);
	
	ts->current_time = now;
	ts->accumulator  = 0;
	ts->lerp_factor  = 0;
}

void
pxl_stepper_sync_time(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->current_time > 0);
	
	double dt = now - ts->current_time;
	dt = (dt > 0.25) ? 0.25 : dt;
	
	ts->accumulator += dt;
	ts->current_time = now;
}

bool
pxl_stepper_advance(pxl_time_stepper_t *ts) {
	assert(ts);
	assert(ts->dt > 0);
	
	if (ts->accumulator >= ts->dt) {
		ts->accumulator -= ts->dt;
		return true;
	}
	
	assert(ts->dt > 0);
	ts->lerp_factor = (float)(ts->accumulator / ts->dt);
	return false;
}


#endif /* PXL_STEPPER_H */
