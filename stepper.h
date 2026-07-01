#ifndef STEPPER_H
#define STEPPER_H

#include <assert.h>
#include <stdbool.h>

typedef struct {
	double dt;
    float  lerp_factor;

    double current_time;
    double accumulator;
} time_stepper_t;

void
stepper_init(time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->dt > 0);
	
	ts->current_time = now;
	ts->accumulator  = 0;
	ts->lerp_factor  = 0;
}

void
stepper_sync_time(time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->current_time > 0);
	
	double dt = now - ts->current_time;
    dt = (dt > 0.25) ? 0.25 : dt;
	
	ts->accumulator += dt;
    ts->current_time = now;
}

bool
stepper_advance(time_stepper_t *ts) {
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


#endif /* STEPPER_H */
