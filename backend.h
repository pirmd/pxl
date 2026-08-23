#ifndef PXL_BACKEND_H
#define PXL_BACKEND_H

#include <stdbool.h>
#include "err.h"
#include "buf.h"
#include "input.h"

/*
 * When adding a new backend, ensure that:
 *   . All backends MUST use ARGB8888 pixel format (little-endian: A:24-31,
 *     R:16-23, G:8-15, B:0-7) to be consistent with PXL's native color format
 *   . All backends must return pixel-aligned stride in out_pb->stride
 *     (i.e., out_pb->stride * sizeof(pxl_t) must be a valid memory offset)
 *     This has to be enforced by checks in backend implementations.
 */

/* Initialize the backend */
pxl_err_t
pxl_backend_init(const char *title, int w, int h, bool fullscreen);

/* Cleanup the backend */
void
pxl_backend_deinit(void);

/* Begin frame - fill out_pb with drawable memory */
pxl_err_t
pxl_backend_begin_frame(pxl_buf_t *out_pb);

/* End frame - present to screen */
void
pxl_backend_end_frame(void);

/* Get time returns monotonically increasing time in seconds since start of
 * backend
 */
double
pxl_backend_get_time(void);

/* Poll events - drains the event queue and updates the provided input state.
 *
 * The pxl_input_t struct passed as argument MUST be zero-initialized before first use
 * (e.g., pxl_input_t in = {0};). Backends only UPDATE its fields:
 *   - key/mouse button states (via state[] bitset)
 *   - mouse_x, mouse_y (absolute window coordinates, or -1 if not in window)
 *   - mouse_wheel_x/y (delta since last poll)
 */
void
pxl_backend_poll_events(pxl_input_t *in);

#endif /* PXL_BACKEND_H */
