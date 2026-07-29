#ifndef PXL_DRAW_H
#define PXL_DRAW_H

#include "bitmask.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Outline primitives */
void pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1);
void pxl_draw_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

/* Filled primitives */
void pxl_fill_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

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

/* Internal helper: draw a horizontal span respecting scissor X only.
 * Y must be pre-clipped by caller. Does NOT apply canvas offset.
 * Used internally by other drawing primitives (e.g., circle, triangle).
 * Not part of the public API - users should prefer higher-level primitives.
 */
static inline void
pxl_draw_span(pxl_canvas_t *cnv, int x, int y, int w) {
	assert(cnv && cnv->pb);
	assert(w >= 0);
	assert(y >= cnv->scissor.y && y < cnv->scissor.y + cnv->scissor.h);

	pxl_span_t span;
	if (!pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){cnv->scissor.x, cnv->scissor.w}, &span)) {
		return;
	}

	pxl_t pix = cnv->color;
	pxl_t *row = pxl_buf_ptr(cnv->pb, span.x, y);
	for (int dx = 0; dx < span.w; ++dx) {
		row[dx] = pix;
	}
}

#endif /* PXL_DRAW_H */
