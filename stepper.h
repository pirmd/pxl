#ifndef PXL_STEPPER_H
#define PXL_STEPPER_H

#include <assert.h>
#include <stdbool.h>

/* Time-stepped physics loop helper for fixed timestep with interpolation.
   Used to decouple game logic updates from rendering frame rate. */
typedef struct {
	double dt;            /* Fixed timestep (seconds) - must be > 0 */
	float  lerp_factor;   /* Current interpolation factor between frames [0, 1) */

	double current_time;  /* Current time (seconds) since backend init */
	double accumulator;   /* Accumulated time for fixed steps */
} pxl_time_stepper_t;

/* Initialize time stepper with current time.
   ts->dt must be set to a positive value before calling. */
void
pxl_stepper_init(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->dt > 0);
	
	ts->current_time = now;
	ts->accumulator  = 0;
	ts->lerp_factor  = 0;
}

/* Sync time stepper with current time (clamp max frame time to 0.25s).
   Call this at the start of each frame before pxl_stepper_advance(). */
void
pxl_stepper_sync_time(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->current_time > 0);
	
	double dt = now - ts->current_time;
	dt = (dt > 0.25) ? 0.25 : dt;
	
	ts->accumulator += dt;
	ts->current_time = now;
}

/* Advance time stepper by one fixed step.
   Returns true if a full dt step has accumulated (update game logic).
   Returns false if interpolating (render intermediate state).
   Sets lerp_factor for interpolation between frames. */
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
