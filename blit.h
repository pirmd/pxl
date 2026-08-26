#ifndef PXL_BLIT_H
#define PXL_BLIT_H

#include "bitmask.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Blit a rectangle from a pixel buffer to the canvas at (cnv_x, cnv_y).
 * Caller must ensure pb_r is within pb bounds (asserted).
 */
void pxl_blit_rect(pxl_canvas_t *cnv, const pxl_buf_t *pb,
		pxl_rect_t pb_r, int cnv_x, int cnv_y);

/* Draw a region from bitmask to canvas at (cnv_x, cnv_y).
 * Pixels where bitmask bit is 1 are drawn with canvas color.
 * Caller must ensure bm_r is within bm bounds (asserted).
 */
void pxl_draw_bitmask(pxl_canvas_t *cnv, const pxl_bitmask_t *bm,
		pxl_rect_t bm_r, int cnv_x, int cnv_y);

#endif /* PXL_BLIT_H */
