#ifndef PXL_STEPPER_H
#define PXL_STEPPER_H

#include <assert.h>
#include <stdbool.h>

/* Default maximum frame time to avoid spiral of death on frame drops. */
#define PXL_STEPPER_MAX_FRAME_TIME 0.25

/* Time-stepped physics loop helper for fixed timestep with interpolation.
   Used to decouple game logic updates from rendering frame rate.
   
   Typical usage:
   - Call pxl_stepper_sync_time() at the start of each frame with current time.
   - Call pxl_stepper_advance() in a loop until it returns false to update logic.
   - Use lerp_factor for rendering interpolation between logic updates.
   - Set paused=true to freeze time accumulation (useful for pause menus).
   - Adjust time_scale for slow-motion (0.5) or fast-forward (2.0). */
typedef struct {
	double dt;            /* Fixed timestep (seconds) - must be > 0 */
	float  lerp_factor;   /* Current interpolation factor between frames [0, 1) */

	double current_time;  /* Current time (seconds) since backend init */
	double accumulator;   /* Accumulated time for fixed steps */
	bool   paused;        /* If true, stepper won't accumulate time (but current_time still updates) */
	float  time_scale;    /* Time scale factor: 1.0=normal, 0.5=slow-mo, 2.0=fast-forward */
} pxl_time_stepper_t;

/* Initialize time stepper with current time.
   ts->dt must be set to a positive value before calling. */
static inline void
pxl_stepper_init(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->dt > 0);
	
	ts->current_time = now;
	ts->accumulator  = 0;
	ts->lerp_factor  = 0;
	ts->paused       = false;
	ts->time_scale   = 1.0f;
}

/* Reinitialize time stepper with current time.
   Equivalent to pxl_stepper_init(). Provided for semantic clarity. */
static inline void
pxl_stepper_reinit(pxl_time_stepper_t *ts, double now) {
	pxl_stepper_init(ts, now);
}

/* Sync time stepper with current time (clamps max frame time to PXL_STEPPER_MAX_FRAME_TIME).
   Call this at the start of each frame before pxl_stepper_advance().
   If paused, only updates current_time without accumulating. */
static inline void
pxl_stepper_sync_time(pxl_time_stepper_t *ts, double now) {
	assert(ts);
	assert(ts->current_time >= 0);
	
	if (ts->paused) {
		ts->current_time = now;
		return;
	}
	
	double frame_dt = now - ts->current_time;
	if (frame_dt > PXL_STEPPER_MAX_FRAME_TIME)
		frame_dt = PXL_STEPPER_MAX_FRAME_TIME;
	
	ts->accumulator += frame_dt * ts->time_scale;
	ts->current_time = now;
}

/* Advance time stepper by one fixed step.
   Returns true if a full dt step has accumulated (update game logic).
   Returns false if interpolating (render intermediate state).
   Sets lerp_factor for interpolation between frames. */
static inline bool
pxl_stepper_advance(pxl_time_stepper_t *ts) {
	assert(ts);
	assert(ts->dt > 0);
	
	if (ts->accumulator >= ts->dt) {
		ts->accumulator -= ts->dt;
		return true;
	}
	
	ts->lerp_factor = (float)(ts->accumulator / ts->dt);
	return false;
}

#endif /* PXL_STEPPER_H */
