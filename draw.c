#include <assert.h>

#include "draw.h"

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

	if (pxl_canvas_quick_reject(cnv, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1)) {
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
