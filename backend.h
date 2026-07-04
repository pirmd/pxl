#ifndef PXL_BACKEND_H
#define PXL_BACKEND_H

#include <stdbool.h>
#include "canvas.h"
#include "input.h"
#include "err.h"

/*
 * Adding a new backend:
 *   . All backends must return pixel-aligned stride in out_pb->stride
 *     (i.e., out_pb->stride * sizeof(pxl_t) must be a valid memory offset)
 *     This has to be enforced by checks in backend implementations.
 *   . Update Makefile to support compile-time correct dependancies
 *   . Input: pxl_backend_poll_events() must fill in->pressed[] with key codes
 *     from input.h (PXL_KEYB_A, PXL_KEYB_1, etc.). Key codes represent
 *     physical key positions (US layout reference). Backends must handle
 *     all keyboard layouts (QWERTY, AZERTY, etc.) by mapping native codes
 *     to these constants.
 */

// Initialize the backend
pxl_err_t
pxl_backend_init(const char *title, int w, int h, bool fullscreen);

// Cleanup the backend
void
pxl_backend_deinit(void);

// Begin frame - fill out_pb with drawable memory
pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb);

// End frame - present to screen
void
pxl_backend_end_frame(void);

// Get time since start of the backend
double
pxl_backend_get_time(void);

// Poll events - drains the event queue
void
pxl_backend_poll_events(pxl_input_state_t *in);

#endif /* PXL_BACKEND_H */
