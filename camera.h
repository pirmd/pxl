#ifndef PXL_CAMERA_H
#define PXL_CAMERA_H

#include "canvas.h"

/*
 * Camera helpers: offset-based camera management for pxl_canvas_t.
 * Uses the canvas offset to represent camera position.
 */

/* Movement ---------------------------------------------------------------- */
static inline void
pxl_canvas_move_camera(pxl_canvas_t *cnv, int dx, int dy) {
	cnv->offset_x += dx;
	cnv->offset_y += dy;
}

static inline void
pxl_canvas_set_camera(pxl_canvas_t *cnv, int x, int y) {
	cnv->offset_x = x;
	cnv->offset_y = y;
}

static inline void
pxl_canvas_reset_camera(pxl_canvas_t *cnv) {
	cnv->offset_x = 0;
	cnv->offset_y = 0;
}

#endif /* PXL_CAMERA_H */
