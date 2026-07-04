#include <assert.h>

#include "draw.h"

/* Line ------------------------------------------------------------------ */
void
pxl_draw_line(pxl_canvas_t *cnv, int x0, int y0, int x1, int y1) {
	assert(cnv);

	/* Bounding box */
	int min_x = pxl_min(x0, x1);
	int min_y = pxl_min(y0, y1);
	int max_x = pxl_max(x0, x1);
	int max_y = pxl_max(y0, y1);

	/* Quick reject */
	if (pxl_canvas_quick_reject(cnv, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)) {
		return;
	}

	/* Clip line endpoints to scissor Y bounds */
	int sc_y1 = cnv->scissor.y;
	int sc_y2 = cnv->scissor.y + cnv->scissor.h - 1;

	/* Only clip Y if line is not horizontal (y0 != y1) */
	if (y0 != y1) {
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
	}

	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {  /* X-major line */
		int err = dx / 2;
		for (;;) {
			/* y0 guaranteed in scissor by clipping above */
			pxl_draw_span(cnv, x0, y0, 1);
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
			pxl_draw_span(cnv, x0, y0, 1);
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

	if (w <= 0 || h <= 0) {
		return;
	}

	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	pxl_t  color = cnv->color;
	int   stride = cnv->pb->stride;

	// Draw top border (if visible)
	if (r.y == y) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}

	if (r.h == 1) return;

	// Draw bottom border (if visible)
	int bottom_y = r.y + r.h;
	if (bottom_y == y + h) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, bottom_y - 1);
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
	}

	// Draw left border (if visible)
	if (r.x == x) {
		pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
		for (int dy = 0; dy < r.h; ++dy) {
			*row  = color;
			 row += stride;
		}
	}

	if (r.w == 1) return;

	// Draw right border (if visible)
	int right_x = r.x + r.w;
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

	if (w <= 0 || h <= 0) {
		return;
	}

	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){x, y, w, h}, cnv->scissor, &r)) {
		return;
	}

	pxl_t color = cnv->color;
	pxl_t *row = pxl_buf_ptr(cnv->pb, r.x, r.y);
	int stride = cnv->pb->stride;

	for (int dy = 0; dy < r.h; dy++) {
		for (int dx = 0; dx < r.w; dx++) {
			row[dx] = color;
		}
		row += stride;
	}
}
