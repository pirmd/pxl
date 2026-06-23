#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include "pixbuf.h"
#include "input.h"
#include "err.h"

/* Backend API ----------------------------------------------------------- */
/*
 * Adding a new backend:
 *   . Create backend_<name>.c implementing the 5 functions
 *   . Update Makefile to support compile-time correct dependancies
 * 
 * Implementation Notes:
 *   - ALL backends must return pixel-aligned stride in out_pb->stride
 *     (i.e., out_pb->stride * sizeof(pix_t) must be a valid memory offset)
 *     This has to be enforced by checks in backend implementations.
 */

// Initialize the backend
pxl_err_t
backend_init(const char *title, int w, int h, bool fullscreen);

// Cleanup the backend
void
backend_deinit(void);

// Begin frame - fill out_pb with drawable memory
pxl_err_t
backend_begin_frame(pixbuf_t *out_pb);

// End frame - present to screen
void
backend_end_frame(void);

// Get time since start of the backend
double
backend_get_time(void);

// Poll events - update input state, returns false on quit
bool
backend_poll_events(input_t *input);

/* Clock management for main loop control -------------------------------- */
#define FIXED_FPS 60
static const double FIXED_DT = 1.0 / FIXED_FPS;

static struct {
    double current_time;
    double accumulator;
    float  alpha;
} g_clock = {0};

static void
backend_new_frame(void) {
    double   new_time = backend_get_time(); 
    double frame_time = new_time - g_clock.current_time;
    g_clock.current_time = new_time;

    if (frame_time > 0.25) frame_time = 0.25;

    g_clock.accumulator += frame_time;
}

static bool
backend_next_physics_step(float *out_dt) {
    if (g_clock.accumulator >= FIXED_DT) {
        g_clock.accumulator -= FIXED_DT;
        *out_dt = (float)FIXED_DT;
        return true;
    }

    g_clock.alpha = (float)(g_clock.accumulator / FIXED_DT);
    return false;
}

static float
backend_get_alpha(void) {
   	return g_clock.alpha;
}

#endif /* BACKEND_H */
