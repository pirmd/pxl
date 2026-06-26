#ifndef DRAW2D_H
#define DRAW2D_H

#include "canvas.h"

/* Outline primitives */
void draw2d_line(canvas_t *cnv, int x0, int y0, int x1, int y1);
void draw2d_rect(canvas_t *cnv, int x, int y, int w, int h);
void draw2d_circle(canvas_t *cnv, int x, int y, int r);
void draw2d_triangle(canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

/* Filled primitives */
void draw2d_fill_rect(canvas_t *cnv, int x, int y, int w, int h);
void draw2d_fill_circle(canvas_t *cnv, int x, int y, int r);
void draw2d_fill_triangle(canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2);

#endif /* DRAW2D_H */
