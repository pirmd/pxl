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

/* Flip flags for transformations. */
typedef enum {
    PXL_FLIP_NONE = 0,        /* No flip */
    PXL_FLIP_H    = (1 << 0), /* Flip horizontally */
    PXL_FLIP_V    = (1 << 1), /* Flip vertically */
} pxl_flip_t;

/* Draw a bitmask region with optional scaling and flipping.
 * scale must be >= 1.
 * If scale=1 and flip=PXL_FLIP_NONE, behaves like pxl_draw_bitmask().
 * Uses nearest-neighbor scaling. Respects canvas offset and scissor.
 */
void pxl_draw_bitmask_transformed(pxl_canvas_t *cnv, const pxl_bitmask_t *bm,
        pxl_rect_t bm_r, int x, int y, int scale, pxl_flip_t flip);

/* Blit a rectangle from a pixel buffer with optional scaling and flipping.
 * scale must be >= 1.
 * If scale=1 and flip=PXL_FLIP_NONE, behaves like pxl_blit_rect().
 * Respects canvas offset and scissor.
 */
void pxl_blit_transformed(pxl_canvas_t *cnv, const pxl_buf_t *pb,
        pxl_rect_t pb_r, int x, int y, int scale, pxl_flip_t flip);

#endif /* PXL_BLIT_H */
