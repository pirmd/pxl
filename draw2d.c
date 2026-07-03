#include <assert.h>
#include <stdlib.h>

#include "draw2d.h"
#include "geom.h"
#include "pixbuf.h"

/* draw a horizontal span respecting scissor X (Y must be pre-clipped by caller) */
static inline void
draw_span(canvas_t *cnv, int x, int y, int w) {
	assert(cnv && cnv->pb);
	assert(w >= 0);
	assert(y >= cnv->scissor.y && y < cnv->scissor.y + cnv->scissor.h);

	span_t span;
	if (!clip_span((span_t){x, w}, (span_t){cnv->scissor.x, cnv->scissor.w}, &span)) {
		return;
	}

	pix_t pix = cnv->color;
	pix_t *row = pixbuf_ptr(cnv->pb, span.x, y);
	for (int dx = 0; dx < span.w; ++dx) {
		row[dx] = pix;
	}
}

/* Rectangle ------------------------------------------------------------- */
void
draw2d_rect(canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv);

	if (w <= 0 || h <= 0) {
		return;
	}

	rect_t r;
	if (!clip_rect((rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	pix_t  color = cnv->color;
	int   stride = cnv->pb->stride;

	// Draw top border (if visible)
	if (r.y == y) {
		pix_t *row = pixbuf_ptr(cnv->pb, r.x, r.y);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}
	
	if (r.h == 1) return;
	
	// Draw bottom border (if visible)
	int bottom_y = r.y + r.h;
	if (bottom_y == y + h) {
		pix_t *row = pixbuf_ptr(cnv->pb, r.x, bottom_y - 1);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}

	// Draw left border (if visible)
	if (r.x == x) {
		pix_t *row = pixbuf_ptr(cnv->pb, r.x, r.y);
		for (int dy = 0; dy < r.h; ++dy) {
			*row  = color;
			 row += stride;
		}
	}

	if (r.w == 1) return;
	
	// Draw right border (if visible)
	int right_x = r.x + r.w;
	if (right_x == x + w) {
		pix_t *row = pixbuf_ptr(cnv->pb, right_x - 1, r.y);
		for (int dy = 0; dy < r.h; ++dy) {
			*row  = color;
			 row += stride;
		}
	}
}

void
draw2d_fill_rect(canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv);

	if (w <= 0 || h <= 0) {
		return;
	}

	rect_t r;
	if (!clip_rect((rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	pix_t color = cnv->color;
	pix_t *row = pixbuf_ptr(cnv->pb, r.x, r.y);
	int stride = cnv->pb->stride;

	for (int dy = 0; dy < r.h; dy++) {
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
		row += stride;
	}
}

/* Line ------------------------------------------------------------------ */
void
draw2d_line(canvas_t *cnv, int x0, int y0, int x1, int y1) {
	assert(cnv);

	/* Bounding box */
	int min_x = pxl_min(x0, x1);
	int min_y = pxl_min(y0, y1);
	int max_x = pxl_max(x0, x1);
	int max_y = pxl_max(y0, y1);

	/* Quick reject */
	if (canvas_quick_reject(cnv, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)) {
		return;
	}

	/* Clip line endpoints to scissor Y bounds */
	int sc_y1 = cnv->scissor.y;
	int sc_y2 = cnv->scissor.y + cnv->scissor.h - 1;
	
	if (y0 < sc_y1) {
		if (y1 < sc_y1) return;  /* Entire line outside */
		/* Interpolate x0 at y = sc_y1 */
		float t = (float)(sc_y1 - y0) / (float)(y1 - y0);
		x0 = x0 + (int)((float)(x1 - x0) * t + 0.5f);
		y0 = sc_y1;
	}
	if (y0 > sc_y2) {
		if (y1 > sc_y2) return;
		float t = (float)(sc_y2 - y0) / (float)(y1 - y0);
		x0 = x0 + (int)((float)(x1 - x0) * t + 0.5f);
		y0 = sc_y2;
	}
	if (y1 < sc_y1) {
		/* y0 >= sc_y1 guaranteed by above */
		float t = (float)(sc_y1 - y1) / (float)(y0 - y1);
		x1 = x1 + (int)((float)(x0 - x1) * t + 0.5f);
		y1 = sc_y1;
	}
	if (y1 > sc_y2) {
		/* y0 <= sc_y2 guaranteed by above */
		float t = (float)(sc_y2 - y1) / (float)(y0 - y1);
		x1 = x1 + (int)((float)(x0 - x1) * t + 0.5f);
		y1 = sc_y2;
	}

	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {  /* X-major line */
		int err = dx / 2;
		for (;;) {
			/* y0 guaranteed in scissor by clipping above */
			draw_span(cnv, x0, y0, 1);
			if (x0 == x1 && y0 == y1) break;
			x0 += sx;
			err -= dy;
			if (err < 0) {
				y0 += sy;
				err += dx;
			}
		}
	} else {  /* Y-major line */
		int err = dy / 2;
		for (;;) {
			/* y0 guaranteed in scissor by clipping above */
			draw_span(cnv, x0, y0, 1);
			if (x0 == x1 && y0 == y1) break;
			y0 += sy;
			err -= dx;
			if (err < 0) {
				x0 += sx;
				err += dy;
			}
		}
	}
}

/* Circle -------------------------------------------------------------- */
void
draw2d_circle(canvas_t *cnv, int x, int y, int r) {
	assert(cnv);

	if (r <= 0) {
		return;
	}

	/* Bounding box and clip to scissor to get valid Y range */
	rect_t bbox = {x - r, y - r, 2 * r + 1, 2 * r + 1};
	rect_t clipped;
	if (!clip_rect(bbox, cnv->scissor, &clipped)) {
		return;
	}
	int y_start = clipped.y;
	int y_end = clipped.y + clipped.h - 1;

	/* Bresenham's circle algorithm drawing 8 symmetric points */
	int cx = r;
	int cy = 0;
	int df = 1 - r;
	int d_e = 3;
	int d_se = -2 * r + 5;

	/* Starting point (r, 0) - all have cy=0 so y+cy = y-cy = y */
	if (y >= y_start && y <= y_end) {
		draw_span(cnv, x + cx, y + cy, 1);
		draw_span(cnv, x + cy, y + cx, 1);
		draw_span(cnv, x - cx, y + cy, 1);
		draw_span(cnv, x - cy, y + cx, 1);
		draw_span(cnv, x + cx, y - cy, 1);
		draw_span(cnv, x + cy, y - cx, 1);
		draw_span(cnv, x - cx, y - cy, 1);
		draw_span(cnv, x - cy, y - cx, 1);
	}

	cx--;
	while (cy < cx) {
		if (df < 0) {
			df += d_e;
			d_e += 2;
		} else {
			df += d_se;
			d_e += 2;
			d_se += 2;
			cx--;
		}
		cy++;

		/* Draw 8 symmetric points - check each y coordinate */
		if (y + cy >= y_start && y + cy <= y_end) {
			draw_span(cnv, x + cx, y + cy, 1);
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			draw_span(cnv, x + cy, y + cx, 1);
		}
		if (y + cy >= y_start && y + cy <= y_end) {
			draw_span(cnv, x - cx, y + cy, 1);
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			draw_span(cnv, x - cy, y + cx, 1);
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			draw_span(cnv, x + cx, y - cy, 1);
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			draw_span(cnv, x + cy, y - cx, 1);
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			draw_span(cnv, x - cx, y - cy, 1);
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			draw_span(cnv, x - cy, y - cx, 1);
		}
	}
}

void
draw2d_fill_circle(canvas_t *cnv, int x, int y, int r) {
	assert(cnv);

	if (r <= 0) {
		return;
	}

	rect_t bbox = {x - r, y - r, 2 * r + 1, 2 * r + 1};
	rect_t clipped;
	if (!clip_rect(bbox, cnv->scissor, &clipped)) {
		return;
	}

	int y_top = clipped.y;
	int y_bot = clipped.y + clipped.h - 1;

	int cx = r;
	int cy = 0;
	int d = 1 - r;

	while (cx >= cy) {
		int w1 = 2 * cx + 1;
		int w2 = 2 * cy + 1;

		int yy1 = y + cy;
		int yy2 = y - cy;
		int yy3 = y + cx;
		int yy4 = y - cx;

		if (yy1 >= y_top && yy1 <= y_bot) {
			draw_span(cnv, x - cx, yy1, w1);
		}
		if (cy != 0 && yy2 >= y_top && yy2 <= y_bot) {
			draw_span(cnv, x - cx, yy2, w1);
		}

		if (cx != cy) {
			if (yy3 >= y_top && yy3 <= y_bot) {
				draw_span(cnv, x - cy, yy3, w2);
			}
			if (cx != 0 && yy4 >= y_top && yy4 <= y_bot) {
				draw_span(cnv, x - cy, yy4, w2);
			}
		}

		cy++;
		if (d < 0) {
			d += 2 * cy + 1;
		} else {
			cx--;
			d += 2 * (cy - cx) + 1;
		}
	}
}

/* Triangle ------------------------------------------------------------- */
void
draw2d_triangle(canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	assert(cnv);

	/* Use lines for triangle outline */
	draw2d_line(cnv, x0, y0, x1, y1);
	draw2d_line(cnv, x1, y1, x2, y2);
	draw2d_line(cnv, x2, y2, x0, y0);
}

void
draw2d_fill_triangle(canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	assert(cnv);

	/* Bounding box */
	int min_x = pxl_min(pxl_min(x0, x1), x2);
	int min_y = pxl_min(pxl_min(y0, y1), y2);
	int max_x = pxl_max(pxl_max(x0, x1), x2);
	int max_y = pxl_max(pxl_max(y0, y1), y2);

	/* Clip bbox to scissor to get valid Y range */
	rect_t bbox = {min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
	rect_t clipped;
	if (!clip_rect(bbox, cnv->scissor, &clipped)) {
		return;
	}

	/* Scanline algorithm: iterate only over clipped Y range */
	for (int y = clipped.y; y < clipped.y + clipped.h; y++) {
		float x_min = (float)max_x + 1.0f;
		float x_max = (float)min_x - 1.0f;

		/* Check intersection with each edge */
		/* Edge 0-1: only if not horizontal */
		if (y0 != y1) {
			float y_clamped = (y < pxl_min(y0, y1)) ? (float)pxl_min(y0, y1) :
			                  (y > pxl_max(y0, y1)) ? (float)pxl_max(y0, y1) : (float)y;
			float t = (y_clamped - (float)y0) / (float)(y1 - y0);
			float x = (float)x0 + (float)(x1 - x0) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Edge 1-2: only if not horizontal */
		if (y1 != y2) {
			float y_clamped = (y < pxl_min(y1, y2)) ? (float)pxl_min(y1, y2) :
			                  (y > pxl_max(y1, y2)) ? (float)pxl_max(y1, y2) : (float)y;
			float t = (y_clamped - (float)y1) / (float)(y2 - y1);
			float x = (float)x1 + (float)(x2 - x1) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Edge 2-0: only if not horizontal */
		if (y2 != y0) {
			float y_clamped = (y < pxl_min(y2, y0)) ? (float)pxl_min(y2, y0) :
			                  (y > pxl_max(y2, y0)) ? (float)pxl_max(y2, y0) : (float)y;
			float t = (y_clamped - (float)y2) / (float)(y0 - y2);
			float x = (float)x2 + (float)(x0 - x2) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Include vertices that lie exactly on this y */
		if (y0 == y) { if ((float)x0 < x_min) x_min = (float)x0; if ((float)x0 > x_max) x_max = (float)x0; }
		if (y1 == y) { if ((float)x1 < x_min) x_min = (float)x1; if ((float)x1 > x_max) x_max = (float)x1; }
		if (y2 == y) { if ((float)x2 < x_min) x_min = (float)x2; if ((float)x2 > x_max) x_max = (float)x2; }

		/* Draw the span if valid */
		if (x_min <= x_max) {
			/* Round to nearest integer, with bias on right to avoid overdraw */
			int left = (int)(x_min + 0.5f);
			int right = (int)(x_max + 0.499f);
			int span_w = right - left + 1;
			if (span_w > 0) {
				draw_span(cnv, left, y, span_w);
			}
		}
	}
}

