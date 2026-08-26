#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Internal helper: draw a horizontal span respecting scissor X only.
 * Y must be pre-clipped by caller. Does NOT apply canvas offset.
 * Used internally by other drawing primitives (e.g., circle, triangle).
 */
static void
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

/* Line ------------------------------------------------------------------ */
void
pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1) {
	assert(cnv);

	x0 += cnv->offset_x;
	y0 += cnv->offset_y;
	x1 += cnv->offset_x;
	y1 += cnv->offset_y;
	
	/* Bounding box */
	const int min_x = pxl_min(x0, x1);
	const int min_y = pxl_min(y0, y1);
	const int max_x = pxl_max(x0, x1);
	const int max_y = pxl_max(y0, y1);

	/* Quick reject: bounding box completely outside scissor */
	const pxl_rect_t *sc = &cnv->scissor;
	if (min_x >= sc->x + sc->w || max_x < sc->x ||
	    min_y >= sc->y + sc->h || max_y < sc->y) {
		return;
	}

	/* Clip line endpoints to scissor Y bounds */
	const int sc_y1 = cnv->scissor.y;
	const int sc_y2 = cnv->scissor.y + cnv->scissor.h - 1;

	if (y0 != y1) {
		if (y0 < sc_y1) {
			if (y1 < sc_y1) return;  /* Entire line outside */
			/* Interpolate x0 at y = sc_y1 */
			x0 += ((x1 - x0) * (sc_y1 - y0) + (y1 - y0) / 2) / (y1 - y0);
			y0 = sc_y1;
		}
		if (y0 > sc_y2) {
			if (y1 > sc_y2) return;
			x0 += ((x1 - x0) * (sc_y2 - y0) + (y1 - y0) / 2) / (y1 - y0);
			y0 = sc_y2;
		}
		if (y1 < sc_y1) {
			/* y0 >= sc_y1 guaranteed by above */
			x1 += ((x0 - x1) * (sc_y1 - y1) + (y0 - y1) / 2) / (y0 - y1);
			y1 = sc_y1;
		}
		if (y1 > sc_y2) {
			/* y0 <= sc_y2 guaranteed by above */
			x1 += ((x0 - x1) * (sc_y2 - y1) + (y0 - y1) / 2) / (y0 - y1);
			y1 = sc_y2;
		}
	}

	const int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	const int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {  /* X-major line */
		int err = dx / 2;
		int sc_x1 = cnv->scissor.x;
		int sc_x2 = cnv->scissor.x + cnv->scissor.w;
		for (;;) {
			/* Direct pixel access: y0 guaranteed in scissor by clipping above */
			if (x0 >= sc_x1 && x0 < sc_x2) {
				*pxl_buf_ptr(cnv->pb, x0, y0) = cnv->color;
			}
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
		int sc_x1 = cnv->scissor.x;
		int sc_x2 = cnv->scissor.x + cnv->scissor.w;
		for (;;) {
			/* Direct pixel access: y0 guaranteed in scissor by clipping above */
			if (x0 >= sc_x1 && x0 < sc_x2) {
				*pxl_buf_ptr(cnv->pb, x0, y0) = cnv->color;
			}
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

/* Rectangle ------------------------------------------------------------- */
void
pxl_draw_rect(pxl_canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv);
	assert(w > 0 && h > 0);

	x += cnv->offset_x;
	y += cnv->offset_y;

	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	const pxl_t color = cnv->color;
	const int stride = cnv->pb->stride;

	/* Draw top border (if visible) */
	if (r.y == y) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}

	if (r.h == 1) return;

	/* Draw bottom border (if visible) */
	const int bottom_y = r.y + r.h;
	if (bottom_y == y + h) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, bottom_y - 1);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}

	/* Draw left border (if visible) */
	if (r.x == x) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
		for (int dy = 0; dy < r.h; ++dy) {
			*row  = color;
			 row += stride;
		}
	}

	if (r.w == 1) return;

	/* Draw right border (if visible) */
	const int right_x = r.x + r.w;
	if (right_x == x + w) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, right_x - 1, r.y);
		for (int dy = 0; dy < r.h; ++dy) {
			*row  = color;
			 row += stride;
		}
	}
}

void
pxl_fill_rect(pxl_canvas_t *cnv, int x, int y, int w, int h) {
	assert(cnv);
	assert(w > 0 && h > 0);

	x += cnv->offset_x;
	y += cnv->offset_y;

	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	const pxl_t color = cnv->color;
	pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
	const int stride = cnv->pb->stride;

	for (int dy = 0; dy < r.h; dy++) {
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
		row += stride;
	}
}

/* Circle -------------------------------------------------------------- */
void
pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r) {
	assert(cnv);
	assert(r > 0 && r <= (INT_MAX - 1) / 2);

	x += cnv->offset_x;
	y += cnv->offset_y;

	pxl_rect_t bbox;
	if (!pxl_clip_rect((pxl_rect_t){x - r, y - r, 2 * r + 1, 2 * r + 1}, cnv->scissor, &bbox)) {
		return;
	}
	const int y_start = bbox.y;
	const int y_end = bbox.y + bbox.h - 1;
	const int sc_x1 = cnv->scissor.x;
	const int sc_x2 = cnv->scissor.x + cnv->scissor.w;

	/* Bresenham's circle algorithm drawing 8 symmetric points */
	int cx = r;
	int cy = 0;
	int df = 1 - r;
	int d_e = 3;
	int d_se = -2 * r + 5;

	/* Starting point (r, 0) - all have cy=0 so y+cy = y-cy = y */
	if (y >= y_start && y <= y_end) {
		/* Direct pixel access (y already in scissor) */
		if (x + cx >= sc_x1 && x + cx < sc_x2) *pxl_buf_ptr(cnv->pb, x + cx, y + cy) = cnv->color;
		if (x + cy >= sc_x1 && x + cy < sc_x2) *pxl_buf_ptr(cnv->pb, x + cy, y + cx) = cnv->color;
		if (x - cx >= sc_x1 && x - cx < sc_x2) *pxl_buf_ptr(cnv->pb, x - cx, y + cy) = cnv->color;
		if (x - cy >= sc_x1 && x - cy < sc_x2) *pxl_buf_ptr(cnv->pb, x - cy, y + cx) = cnv->color;
		if (x + cx >= sc_x1 && x + cx < sc_x2) *pxl_buf_ptr(cnv->pb, x + cx, y - cy) = cnv->color;
		if (x + cy >= sc_x1 && x + cy < sc_x2) *pxl_buf_ptr(cnv->pb, x + cy, y - cx) = cnv->color;
		if (x - cx >= sc_x1 && x - cx < sc_x2) *pxl_buf_ptr(cnv->pb, x - cx, y - cy) = cnv->color;
		if (x - cy >= sc_x1 && x - cy < sc_x2) *pxl_buf_ptr(cnv->pb, x - cy, y - cx) = cnv->color;
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

		/* Draw 8 symmetric points - check each y coordinate (direct pixel access) */
		if (y + cy >= y_start && y + cy <= y_end) {
			if (x + cx >= sc_x1 && x + cx < sc_x2) *pxl_buf_ptr(cnv->pb, x + cx, y + cy) = cnv->color;
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			if (x + cy >= sc_x1 && x + cy < sc_x2) *pxl_buf_ptr(cnv->pb, x + cy, y + cx) = cnv->color;
		}
		if (y + cy >= y_start && y + cy <= y_end) {
			if (x - cx >= sc_x1 && x - cx < sc_x2) *pxl_buf_ptr(cnv->pb, x - cx, y + cy) = cnv->color;
		}
		if (y + cx >= y_start && y + cx <= y_end) {
			if (x - cy >= sc_x1 && x - cy < sc_x2) *pxl_buf_ptr(cnv->pb, x - cy, y + cx) = cnv->color;
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			if (x + cx >= sc_x1 && x + cx < sc_x2) *pxl_buf_ptr(cnv->pb, x + cx, y - cy) = cnv->color;
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			if (x + cy >= sc_x1 && x + cy < sc_x2) *pxl_buf_ptr(cnv->pb, x + cy, y - cx) = cnv->color;
		}
		if (y - cy >= y_start && y - cy <= y_end) {
			if (x - cx >= sc_x1 && x - cx < sc_x2) *pxl_buf_ptr(cnv->pb, x - cx, y - cy) = cnv->color;
		}
		if (y - cx >= y_start && y - cx <= y_end) {
			if (x - cy >= sc_x1 && x - cy < sc_x2) *pxl_buf_ptr(cnv->pb, x - cy, y - cx) = cnv->color;
		}
	}
}

void
pxl_fill_circle(pxl_canvas_t *cnv, int x, int y, int r) {
	assert(cnv);
	assert(r > 0 && r <= (INT_MAX - 1) / 2);

	x += cnv->offset_x;
	y += cnv->offset_y;

	pxl_rect_t bbox;
	if (!pxl_clip_rect((pxl_rect_t){x - r, y - r, 2 * r + 1, 2 * r + 1}, cnv->scissor, &bbox)) {
		return;
	}

	const int y_top = bbox.y;
	const int y_bot = bbox.y + bbox.h - 1;

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
		if (yy2 >= y_top && yy2 <= y_bot) {
			pxl_draw_span(cnv, x - cx, yy2, w1);
		}

		if (cx != cy) {
			if (yy3 >= y_top && yy3 <= y_bot) {
				pxl_draw_span(cnv, x - cy, yy3, w2);
			}
			if (yy4 >= y_top && yy4 <= y_bot) {
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
pxl_fill_triangle(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	assert(cnv);

	x0 += cnv->offset_x;
	y0 += cnv->offset_y;
	x1 += cnv->offset_x;
	y1 += cnv->offset_y;
	x2 += cnv->offset_x;
	y2 += cnv->offset_y;

	/* Bounding box */
	const int min_x = pxl_min(pxl_min(x0, x1), x2);
	const int min_y = pxl_min(pxl_min(y0, y1), y2);
	const int max_x = pxl_max(pxl_max(x0, x1), x2);
	const int max_y = pxl_max(pxl_max(y0, y1), y2);
	assert(max_x >= min_x && max_x - min_x <= INT_MAX - 1);
	assert(max_y >= min_y && max_y - min_y <= INT_MAX - 1);

	pxl_rect_t bbox;
	if (!pxl_clip_rect((pxl_rect_t){min_x, min_y, max_x - min_x + 1, max_y - min_y + 1}, cnv->scissor, &bbox)) {
		return;
	}

	const float inv_dy01 = (y0 != y1) ? 1.0f / (float)(y1 - y0) : 0.0f;
	const float inv_dy12 = (y1 != y2) ? 1.0f / (float)(y2 - y1) : 0.0f;
	const float inv_dy20 = (y2 != y0) ? 1.0f / (float)(y0 - y2) : 0.0f;

	for (int y = bbox.y; y < bbox.y + bbox.h; y++) {
		float x_min = (float)max_x + 1.0f;
		float x_max = (float)min_x - 1.0f;

		/* Check intersection with each edge */
		/* Edge 0-1: only if not horizontal */
		if (y0 != y1) {
			const int y01_min = pxl_min(y0, y1);
			const int y01_max = pxl_max(y0, y1);
			float y_clamped = (y < y01_min) ? (float)y01_min :
			                  (y > y01_max) ? (float)y01_max : (float)y;
			float t = (y_clamped - (float)y0) * inv_dy01;
			float x = (float)x0 + (float)(x1 - x0) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Edge 1-2: only if not horizontal */
		if (y1 != y2) {
			const int y12_min = pxl_min(y1, y2);
			const int y12_max = pxl_max(y1, y2);
			float y_clamped = (y < y12_min) ? (float)y12_min :
			                  (y > y12_max) ? (float)y12_max : (float)y;
			float t = (y_clamped - (float)y1) * inv_dy12;
			float x = (float)x1 + (float)(x2 - x1) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Edge 2-0: only if not horizontal */
		if (y2 != y0) {
			const int y20_min = pxl_min(y2, y0);
			const int y20_max = pxl_max(y2, y0);
			float y_clamped = (y < y20_min) ? (float)y20_min :
			                  (y > y20_max) ? (float)y20_max : (float)y;
			float t = (y_clamped - (float)y2) * inv_dy20;
			float x = (float)x2 + (float)(x0 - x2) * t;
			if (x < x_min) x_min = x;
			if (x > x_max) x_max = x;
		}

		/* Include vertices that lie exactly on this y */
		if (y0 == y) { if ((float)x0 < x_min) x_min = (float)x0; if ((float)x0 > x_max) x_max = (float)x0; }
		if (y1 == y) { if ((float)x1 < x_min) x_min = (float)x1; if ((float)x1 > x_max) x_max = (float)x1; }
		if (y2 == y) { if ((float)x2 < x_min) x_min = (float)x2; if ((float)x2 > x_max) x_max = (float)x2; }

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

/* Inline triangle outline (uses draw_line) */
static inline void
pxl_draw_triangle_impl(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1, int x2, int y2) {
	pxl_draw_line(cnv, x0, y0, x1, y1);
	pxl_draw_line(cnv, x1, y1, x2, y2);
	pxl_draw_line(cnv, x2, y2, x0, y0);
}
