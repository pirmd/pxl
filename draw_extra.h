#ifndef PXL_DRAW_EXTRA_H
#define PXL_DRAW_EXTRA_H

#include "draw.h"

/* Circle primitives */
void pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r);
void pxl_fill_circle(pxl_canvas_t *cnv, int x, int y, int r);

/* Triangle primitives */
void pxl_draw_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);
void pxl_fill_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

#endif /* PXL_DRAW_EXTRA_H */
