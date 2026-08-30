#ifndef PXL_SHAPE_H
#define PXL_SHAPE_H

#include "canvas.h"

/* Outline primitives */
void pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1);
void pxl_draw_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

/* Filled primitives */
void pxl_fill_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);

/* Circle primitives */
void pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r);
void pxl_fill_circle(pxl_canvas_t *cnv, int x, int y, int r);

/* Triangle primitives */
void pxl_fill_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

static inline void
pxl_draw_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	pxl_draw_line(cnv, x0, y0, x1, y1);
	pxl_draw_line(cnv, x1, y1, x2, y2);
	pxl_draw_line(cnv, x2, y2, x0, y0);
}

#endif /* PXL_SHAPE_H */
