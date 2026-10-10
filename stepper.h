#ifndef PXL_STEPPER_H
#define PXL_STEPPER_H

#include <assert.h>
#include <stdbool.h>

/* Time-stepped physics loop helper for fixed timestep with interpolation.
 *
 * Design:
 *   - ts->dt: Fixed timestep (0 = disabled). Configuration.
 *   - accumulator/alpha: Internal state.
 *   - Control (time_scale, paused) is external: caller computes scaled_frame_dt.
 */
typedef struct {
    double dt;            /* Fixed timestep (seconds). 0 = disabled. */
    double accumulator;   /* Accumulated time for fixed steps. */
    float  alpha;         /* Current interpolation factor [0, 1). */
} pxl_time_stepper_t;

static inline void
pxl_stepper_init(pxl_time_stepper_t *ts, double dt) {
    assert(ts);
    ts->dt = dt;
    ts->accumulator = 0.0;
    ts->alpha = 0.0f;
}

static inline void
pxl_stepper_update(pxl_time_stepper_t *ts, double scaled_frame_dt) {
    assert(ts);
    if (ts->dt <= 0.0) return;
    ts->accumulator += scaled_frame_dt;
}

static inline bool
pxl_stepper_advance(pxl_time_stepper_t *ts) {
    assert(ts);
    if (ts->dt <= 0.0) return false;
    if (ts->accumulator >= ts->dt) {
        ts->accumulator -= ts->dt;
        return true;
    }
    ts->alpha = (float)(ts->accumulator / ts->dt);
    return false;
}

#endif /* PXL_STEPPER_H */
