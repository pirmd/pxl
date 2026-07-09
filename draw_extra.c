#include <assert.h>

#include "draw_extra.h"
#include "geom.h"

/* Circle -------------------------------------------------------------- */
void
pxl_draw_circle(pxl_canvas_t *cnv, int x, int y, int r) {
	assert(cnv);
	assert(r > 0);

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
	assert(r > 0);

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

	pxl_draw_line(cnv, x0, y0, x1, y1);
	pxl_draw_line(cnv, x1, y1, x2, y2);
	pxl_draw_line(cnv, x2, y2, x0, y0);
}

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
