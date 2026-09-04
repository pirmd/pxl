#ifndef PXL_TIMER_H
#define PXL_TIMER_H

#include <assert.h>
#include <stdbool.h>

/* Small epsilon for floating-point comparisons. */
#define PXL_TIMER_EPSILON 1e-9

/* Simple countdown timer for animations, messages, etc.
 *
 * Design:
 *   - remaining: Decremented each frame by scaled_dt.
 *   - initial: Initial duration (for progress calculation).
 *   - Control (time_scale, paused) is external: pass scaled_dt = 0 when paused.
 *
 * Usage:
 *   pxl_timer_t timer;
 *   pxl_timer_start(&timer, 2.0);  // 2-second timer
 *
 *   // In frame loop:
 *   pxl_timer_advance(&timer, app.effective_dt);
 *   if (pxl_timer_finished(&timer)) { play_explosion(); }
 *   float progress = pxl_timer_progress(&timer);
 */
typedef struct {
	double remaining;    /* Time remaining (decremented by scaled_dt). */
	double initial;      /* Initial duration (for progress calculation). */
} pxl_timer_t;

static inline void
pxl_timer_start(pxl_timer_t *t, double duration) {
	assert(t);
	assert(duration > 0.0);
	t->remaining = duration;
	t->initial = duration;
}

static inline bool
pxl_timer_advance(pxl_timer_t *t, double scaled_dt) {
	assert(t);
	if (t->remaining <= 0.0) return false;
	t->remaining -= scaled_dt;
	return t->remaining <= PXL_TIMER_EPSILON;
}

static inline float
pxl_timer_progress(const pxl_timer_t *t) {
	assert(t);
	if (t->initial <= 0.0) return 1.0f;
	if (t->remaining <= PXL_TIMER_EPSILON) return 1.0f;
	return (float)(1.0 - (t->remaining / t->initial));
}

static inline bool
pxl_timer_finished(const pxl_timer_t *t) {
	assert(t);
	return t->remaining <= PXL_TIMER_EPSILON;
}

#endif /* PXL_TIMER_H */
