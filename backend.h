#ifndef BACKEND_H
#define BACKEND_H

#include <stdbool.h>
#include "canvas.h"
#include "input.h"
#include "err.h"

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

// Poll events - drains the event queue
void
backend_poll_events(input_state_t *in);

#endif /* BACKEND_H */
