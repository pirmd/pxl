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

void
pxl_blit_rect(pxl_canvas_t *cnv, const pxl_buf_t *pb,
              pxl_rect_t pb_r, int cnv_x, int cnv_y) {
    assert(cnv && cnv->pb);
    assert(pb && pb->data);
    assert(pb_r.w >= 0 && pb_r.h >= 0);
    assert(pb_r.x >= 0 && pb_r.y >= 0);
    assert(pb_r.x + pb_r.w <= pb->width);
    assert(pb_r.y + pb_r.h <= pb->height);

    cnv_x += cnv->offset_x;
    cnv_y += cnv->offset_y;

    pxl_rect_t dst_rect = {cnv_x, cnv_y, pb_r.w, pb_r.h};

    if (!pxl_clip_rect(dst_rect, cnv->scissor, &dst_rect)) {
        return;
    }

    int src_x = pb_r.x + (dst_rect.x - cnv_x);
    int src_y = pb_r.y + (dst_rect.y - cnv_y);

    const pxl_t *pb_row = pxl_buf_ptr(pb, src_x, src_y);
    int pb_stride = pb->stride;
    pxl_t *cnv_row = pxl_buf_ptr(cnv->pb, dst_rect.x, dst_rect.y);
    int cnv_stride = cnv->pb->stride;

    assert(dst_rect.w <= pb_stride);
    for (int y = 0; y < dst_rect.h; ++y) {
        memcpy(cnv_row, pb_row, dst_rect.w * sizeof(pxl_t));
        pb_row += pb_stride;
        cnv_row += cnv_stride;
    }
}

/* Bitmask drawing -------------------------------------------------------------- */

void
pxl_draw_bitmask(pxl_canvas_t *cnv, const pxl_bitmask_t *bm,
			  pxl_rect_t bm_r, int cnv_x, int cnv_y) {
	assert(cnv && cnv->pb);
	assert(bm && bm->data);
	assert(bm_r.w >= 0 && bm_r.h >= 0);
	assert(bm_r.x >= 0 && bm_r.y >= 0);
	assert(bm_r.x + bm_r.w <= bm->width);
	assert(bm_r.y + bm_r.h <= bm->height);

	cnv_x += cnv->offset_x;
	cnv_y += cnv->offset_y;

	pxl_rect_t dst_rect = {cnv_x, cnv_y, bm_r.w, bm_r.h};
	if (!pxl_clip_rect(dst_rect, cnv->scissor, &dst_rect)) {
		return;
	}

	/* Source offset in bitmask (bit-level) */
	int src_x = bm_r.x + (dst_rect.x - cnv_x);
	int src_y = bm_r.y + (dst_rect.y - cnv_y);

	pxl_t color = cnv->color;

	for (int j = 0; j < dst_rect.h; ++j) {
		pxl_t *dst = pxl_buf_ptr(cnv->pb, dst_rect.x, dst_rect.y + j);
		const uint8_t *m_row = bm->data + ((size_t)(src_y + j) * bm->stride);

		int i = 0;  /* Pixel position in current row */
		size_t bit_offset = (size_t)src_x;  /* Bit offset for current row */

		/* Leading partial byte (if not byte-aligned) */
		if (bit_offset & 0x7) {
			unsigned leading_bit_off = bit_offset & 0x7;
			uint8_t m = m_row[bit_offset >> 3];
			int bits_to_do = (8 - (int)leading_bit_off < dst_rect.w - i) ?
			                 8 - (int)leading_bit_off : dst_rect.w - i;

			for (int bit = 0; bit < bits_to_do; ++bit) {
				if (m & (1U << (leading_bit_off + bit))) {
					dst[i + bit] = color;
				}
			}
			i += bits_to_do;
			bit_offset += bits_to_do;
		}

		/* Full bytes (fast path for 0x00 and 0xFF) */
		for (; i + 8 <= dst_rect.w; i += 8) {
			uint8_t m = m_row[bit_offset >> 3];
			bit_offset += 8;

			if (m == 0x00) {
				continue;  /* Skip 8 pixels */
			} else if (m == 0xFF) {
				for (int bit = 0; bit < 8; ++bit) {
					dst[i + bit] = color;
				}
			} else {
				for (int bit = 0; bit < 8; ++bit) {
					if (m & (1U << bit)) {
						dst[i + bit] = color;
					}
				}
			}
		}

		/* Trailing partial byte */
		int remaining = dst_rect.w - i;
		if (remaining > 0) {
			uint8_t m = m_row[bit_offset >> 3];
			for (int bit = 0; bit < remaining; ++bit) {
				if (m & (1U << bit)) {
					dst[i + bit] = color;
				}
			}
		}
	}
}
