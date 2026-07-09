#ifndef PXL_DRAW_H
#define PXL_DRAW_H

#include "canvas.h"
#include "buf.h"
#include "geom.h"

/* draw a horizontal span respecting scissor X (Y must be pre-clipped by caller) */
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

/* Outline primitives */
void pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1);
void pxl_draw_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

/* Filled primitives */
void pxl_fill_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

/* Blitting primitives */
/* Blit a rectangle from a pixel buffer to the canvas at (cnv_x, cnv_y).
 * Caller must ensure pb_r is within pb bounds (asserted).
 * Destination is clipped to canvas scissor. Respects canvas offset.
 */
void pxl_blit_rect(pxl_canvas_t *cnv, const pxl_buf_t *pb,
		pxl_rect_t pb_r, int cnv_x, int cnv_y);

#endif /* PXL_DRAW_H */
