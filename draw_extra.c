#include <assert.h>
#include <limits.h>

#include "draw_extra.h"
#include "geom.h"

/* Circle -------------------------------------------------------------- */
void
pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r) {
	assert(cnv);
	assert(r > 0);
	assert(x >= -INT_MAX/2 && x <= INT_MAX/2);
	assert(y >= -INT_MAX/2 && y <= INT_MAX/2);

	/* Assert to prevent integer overflow in bbox calculation (extreme case) */
	assert(r <= (INT_MAX / 2) - 1);

	/* Bounding box and clip to scissor to get valid Y range */
	const pxl_rect_t bbox = {x - r, y - r, 2 * r + 1, 2 * r + 1};
	pxl_rect_t clipped;
	if (!pxl_clip_rect(bbox, cnv->scissor, &clipped)) {
		return;
	}
	const int y_start = clipped.y;
	const int y_end = clipped.y + clipped.h - 1;

	/* Bresenham's circle algorithm drawing 8 symmetric points */
	int cx = r;
	int cy = 0;
	int df = 1 - r;
	int d_e = 3;
	int d_se = -2 * r + 5;

	/* Starting point (r, 0) - all have cy=0 so y+cy = y-cy = y */
	if (y >= y_start && y <= y_end) {
		pxl_draw_span(cnv, x + cx, y + cy, 1);
		pxl_draw_span(cnv, x + cy, y + cx, 1);
		pxl_draw_span(cnv, x - cx, y + cy, 1);
		pxl_draw_span(cnv, x - cy, y + cx, 1);
		pxl_draw_span(cnv, x + cx, y - cy, 1);
		pxl_draw_span(cnv, x + cy, y - cx, 1);
		pxl_draw_span(cnv, x - cx, y - cy, 1);
		pxl_draw_span(cnv, x - cy, y - cx, 1);
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
			pxl_draw_span(cnv, x + cx, y + cy, 1);
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			pxl_draw_span(cnv, x + cy, y + cx, 1);
		}
		if (y + cy >= y_start && y + cy <= y_end) {
			pxl_draw_span(cnv, x - cx, y + cy, 1);
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			pxl_draw_span(cnv, x - cy, y + cx, 1);
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			pxl_draw_span(cnv, x + cx, y - cy, 1);
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			pxl_draw_span(cnv, x + cy, y - cx, 1);
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			pxl_draw_span(cnv, x - cx, y - cy, 1);
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			pxl_draw_span(cnv, x - cy, y - cx, 1);
		}
	}
}

void
pxl_fill_circle(pxl_canvas_t *cnv, int x, int y, int r) {
	assert(cnv);
	assert(r > 0);
	assert(x >= -INT_MAX/2 && x <= INT_MAX/2);
	assert(y >= -INT_MAX/2 && y <= INT_MAX/2);

	/* Assert to prevent integer overflow in bbox calculation (extreme case) */
	assert(r <= (INT_MAX / 2) - 1);

	const pxl_rect_t bbox = {x - r, y - r, 2 * r + 1, 2 * r + 1};
	pxl_rect_t clipped;
	if (!pxl_clip_rect(bbox, cnv->scissor, &clipped)) {
		return;
	}

	const int y_top = clipped.y;
	const int y_bot = clipped.y + clipped.h - 1;

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
			pxl_draw_span(cnv, x - cx, yy1, w1);
		}
		if (cy != 0 && yy2 >= y_top && yy2 <= y_bot) {
			pxl_draw_span(cnv, x - cx, yy2, w1);
		}

		if (cx != cy) {
			if (yy3 >= y_top && yy3 <= y_bot) {
				pxl_draw_span(cnv, x - cy, yy3, w2);
			}
			if (cx != 0 && yy4 >= y_top && yy4 <= y_bot) {
				pxl_draw_span(cnv, x - cy, yy4, w2);
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
pxl_draw_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	assert(cnv);

	/* Use lines for triangle outline */
	pxl_draw_line(cnv, x0, y0, x1, y1);
	pxl_draw_line(cnv, x1, y1, x2, y2);
	pxl_draw_line(cnv, x2, y2, x0, y0);
}

void
pxl_fill_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	assert(cnv);

	/* Bounding box */
	const int min_x = pxl_min(pxl_min(x0, x1), x2);
	const int min_y = pxl_min(pxl_min(y0, y1), y2);
	const int max_x = pxl_max(pxl_max(x0, x1), x2);
	const int max_y = pxl_max(pxl_max(y0, y1), y2);

	/* Clip bbox to scissor to get valid Y range */
	const pxl_rect_t bbox = {min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
	pxl_rect_t clipped;
	if (!pxl_clip_rect(bbox, cnv->scissor, &clipped)) {
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
				pxl_draw_span(cnv, left, y, span_w);
			}
		}
	}
}
