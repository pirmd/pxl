#ifndef PXL_DRAW_H
#define PXL_DRAW_H

#include "canvas.h"

/* Outline primitives */
void pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1);
void pxl_draw_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);
void pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r);
void pxl_draw_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

/* Filled primitives */
void pxl_fill_rect(pxl_canvas_t *cnv, int x, int y, int w, int h);
void pxl_fill_circle(pxl_canvas_t *cnv, int x, int y, int r);
void pxl_fill_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

#endif /* PXL_DRAW_H */
