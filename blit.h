#ifndef PXL_BLIT_H
#define PXL_BLIT_H

#include "bitmask.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Rendering: blit and draw operations with optional flip/scale. */

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

/* Draw a bitmask region with optional scaling and flipping.
 * scale must be >= 1.
 * If scale=1 and flip=PXL_FLIP_NONE, behaves like pxl_draw_bitmask().
 * Uses nearest-neighbor scaling. Respects canvas offset and scissor.
 *
 * Example:
 *   // Draw a bitmask at 2x scale
 *   pxl_draw_bitmask_transformed(cnv, &bm, (pxl_rect_t){0,0,8,8}, 10, 10, 2, PXL_FLIP_NONE);
 *   // Draw a bitmask flipped horizontally
 *   pxl_draw_bitmask_transformed(cnv, &bm, (pxl_rect_t){0,0,8,8}, 10, 10, 1, PXL_FLIP_H);
 */
void pxl_draw_bitmask_transformed(pxl_canvas_t *cnv, const pxl_bitmask_t *bm,
		pxl_rect_t bm_r, int x, int y, int scale, pxl_flip_t flip);

/* Blit a rectangle from a pixel buffer with optional scaling and flipping.
 * scale must be >= 1.
 * If scale=1 and flip=PXL_FLIP_NONE, behaves like pxl_blit_rect().
 * Respects canvas offset and scissor.
 *
 * Example:
 *   // Blit a pixel buffer at 2x scale
 *   pxl_blit_transformed(cnv, &pb, (pxl_rect_t){0,0,8,8}, 10, 10, 2, PXL_FLIP_NONE);
 */
void pxl_blit_transformed(pxl_canvas_t *cnv, const pxl_buf_t *pb,
		pxl_rect_t pb_r, int x, int y, int scale, pxl_flip_t flip);

#endif /* PXL_BLIT_H */
