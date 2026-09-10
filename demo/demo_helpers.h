#ifndef PXL_DEMO_HELPERS_H
#define PXL_DEMO_HELPERS_H

#include <assert.h>
#include <stdint.h>
#include <time.h>
#include "pxl.h"

/* =========================================================================
 * PRNG - Portable pseudo-random number generator (LCG)
 * ========================================================================= */

/* Get pointer to internal PRNG state */
static inline uint32_t *demo_rng_state_ptr(void) {
	static uint32_t state = 0;
	return &state;
}

/* Initialize or reset PRNG with a seed (0 = use time) */
static inline void
demo_rng_seed(uint32_t seed) {
	*demo_rng_state_ptr() = seed ? seed : (uint32_t)time(NULL);
}

/* Generate a pseudo-random 32-bit unsigned integer */
static inline uint32_t
demo_rng(void) {
	uint32_t *state = demo_rng_state_ptr();
	*state = *state * 1664525u + 1013904223u;
	return *state;
}

/* Generate a random float in [0, 1) */
static inline float
demo_rng_float(void) {
	return (float)demo_rng() / (float)UINT32_MAX;
}

/* Generate a random float in [min, max) */
static inline float
demo_rng_float_range(float min, float max) {
	return min + (max - min) * demo_rng_float();
}

/* =========================================================================
 * FPS counter
 * ========================================================================= */

/* Update FPS counter */
static inline void
demo_update_fps(double frame_dt, int *current_fps) {
	assert(current_fps != NULL);
	static double accumulator = 0;
	static int frame_count = 0;
	accumulator += frame_dt;
	frame_count++;
	if (accumulator >= 1.0) {
		*current_fps = (int)((float)frame_count / accumulator);
		frame_count = 0;
		accumulator = 0;
	}
}

#endif /* PXL_DEMO_HELPERS_H */
