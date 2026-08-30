#ifndef PXL_DEMO_HELPERS_H
#define PXL_DEMO_HELPERS_H

#include <stdint.h>
#include <time.h>
#include "pxl.h"

/* =========================================================================
 * PRNG - Portable pseudo-random number generator (LCG)
 * ========================================================================= */

/* PRNG state - zero-initialized by default */
extern uint32_t demo_rng_state;

/* Initialize or reset PRNG with a seed (0 = use time) */
static inline void
demo_rng_seed(uint32_t seed) {
	demo_rng_state = seed ? seed : (uint32_t)time(NULL);
}

/* Generate a pseudo-random 32-bit unsigned integer */
static inline uint32_t
demo_rng(void) {
	demo_rng_state = demo_rng_state * 1664525u + 1013904223u;
	return demo_rng_state;
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
 * Scaled drawing utilities (for bitmask fonts)
 * ========================================================================= */

/* Draw a bitmask with pixel scaling.
 *
 * Args:
 *   cnv:     Target canvas
 *   scale:   Scaling factor (1 = original size, 2 = 2x, etc.)
 *   bm:      Source bitmask
 *   bm_r:    Rectangle in the bitmask to draw
 *   cnv_x:   Destination X coordinate
 *   cnv_y:   Destination Y coordinate */
void
demo_draw_bitmask_scaled(pxl_canvas_t *cnv, int scale,
                    const pxl_bitmask_t *bm, pxl_rect_t bm_r,
                    int cnv_x, int cnv_y);

/* Get the scaled bounds (rectangle) of a string using a bitmask font.
 *
 * Args:
 *   font:   Bitmask font to use
 *   str:    String to measure (ASCII only)
 *   scale:  Scaling factor
 *
 * Returns:
 *   Bounding box with width and height (after scaling) */
pxl_rect_t
demo_text_bounds_scaled(const pxl_font_t *font, const char *str, int scale);

/* Draw a string with a bitmask font, scaled by a factor.
 *
 * Args:
 *   cnv:    Target canvas
 *   font:   Bitmask font to use
 *   str:    String to draw (ASCII only)
 *   scale:  Scaling factor
 *   x, y:   Destination coordinates */
void
demo_draw_text_scaled(pxl_canvas_t *cnv, const pxl_font_t *font,
                   const char *str, int scale, int x, int y);

/* =========================================================================
 * Update FPS counter
 * ========================================================================= */

/* Update FPS counter */
void
demo_update_fps(double now, int *current_fps);

#endif /* PXL_DEMO_HELPERS_H */
