#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "blit.h"
#include "bitmask.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

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
    pxl_t *cnv_row = pxl_buf_ptr(cnv->pb, dst_rect.x, dst_rect.y);

    assert(dst_rect.w <= pb->stride);
    for (int y = 0; y < dst_rect.h; ++y) {
        memcpy(cnv_row, pb_row, (size_t)dst_rect.w * sizeof(pxl_t));
        pb_row += pb->stride;
        cnv_row += cnv->pb->stride;
    }
}

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
		const uint8_t *m_row = bm->data + ((size_t)(src_y + j) * (size_t)bm->stride);

		int i = 0;  /* Pixel position in current row */
		size_t bit_offset = (size_t)src_x;  /* Bit offset for current row */

		/* Leading partial byte (if not byte-aligned) */
		if (bit_offset & 0x7) {
			int leading_bit_off = bit_offset & 0x7;
			uint8_t m = m_row[bit_offset >> 3];
			int bits_to_do = (8 - leading_bit_off < dst_rect.w - i) ?
			                 8 - leading_bit_off : dst_rect.w - i;

			for (int bit = 0; bit < bits_to_do; ++bit) {
				if (m & (1U << (leading_bit_off + bit))) {
					dst[i + bit] = color;
				}
			}
			i += bits_to_do;
			bit_offset += (size_t)bits_to_do;
		}

		/* Process full bytes in chunks of 4 (32 pixels at a time) for performance.
		 * Uses memcpy for 0xFF bytes to minimize writes and improve cache locality. */
		pxl_t color8[8] = {color, color, color, color, color, color, color, color};
		for (; i + 32 <= dst_rect.w; i += 32) {
			const uint8_t *m = m_row + (bit_offset >> 3);
			bit_offset += 32;

			/* Process 4 bytes (32 pixels) as a group */
			for (int k = 0; k < 4; ++k) {
				uint8_t byte = m[k];
				if (byte == 0x00) {
					continue;
				} else if (byte == 0xFF) {
					memcpy(dst + i + k * 8, color8, 8 * sizeof(pxl_t));
				} else {
					for (int bit = 0; bit < 8; ++bit) {
						if (byte & (1U << bit)) {
							dst[i + k * 8 + bit] = color;
						}
					}
				}
			}
		}

		/* Process remaining full bytes (8 pixels at a time) */
		for (; i + 8 <= dst_rect.w; i += 8) {
			uint8_t m = m_row[bit_offset >> 3];
			bit_offset += 8;

			if (m == 0x00) {
				continue;
			} else if (m == 0xFF) {
				memcpy(dst + i, color8, 8 * sizeof(pxl_t));
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

void
pxl_draw_bitmask_transformed(pxl_canvas_t *cnv, const pxl_bitmask_t *bm,
		pxl_rect_t bm_r, int x, int y, int scale, pxl_flip_t flip) {
	assert(cnv && cnv->pb);
	assert(bm && bm->data);
	assert(bm_r.w >= 0 && bm_r.h >= 0);
	assert(scale >= 1);

	/* Fast path: no transformation needed */
	if (scale == 1 && flip == PXL_FLIP_NONE) {
		pxl_draw_bitmask(cnv, bm, bm_r, x, y);
		return;
	}

	/* Extract flip flags */
	bool flip_h = (flip & PXL_FLIP_H) != 0;
	bool flip_v = (flip & PXL_FLIP_V) != 0;

	/* Apply canvas offset */
	x += cnv->offset_x;
	y += cnv->offset_y;

	/* Destination rectangle before clipping */
	pxl_rect_t dst_rect = {
		.x = x,
		.y = y,
		.w = bm_r.w * scale,
		.h = bm_r.h * scale
	};

	/* Clip against scissor */
	pxl_rect_t clipped;
	if (!pxl_clip_rect(dst_rect, cnv->scissor, &clipped)) {
		return;  /* Completely outside scissor */
	}

	pxl_t color = cnv->color;

	/* Gather approach: iterate over destination pixels sequentially */
	for (int dst_y = clipped.y; dst_y < clipped.y + clipped.h; dst_y++) {
		pxl_t *dst_row = pxl_buf_ptr(cnv->pb, clipped.x, dst_y);

		for (int dst_x = clipped.x; dst_x < clipped.x + clipped.w; dst_x++) {
			/* Map destination pixel back to source */
			int rel_x = dst_x - x;
			int rel_y = dst_y - y;

			int src_x = flip_h ?
				bm_r.x + bm_r.w - 1 - rel_x / scale :
				bm_r.x + rel_x / scale;
			int src_y = flip_v ?
				bm_r.y + bm_r.h - 1 - rel_y / scale :
				bm_r.y + rel_y / scale;

			/* Check if source pixel is set in bitmask */
			size_t byte_idx = (size_t)src_y * (size_t)bm->stride + (size_t)src_x / 8u;
			uint8_t byte = bm->data[byte_idx];
			int bit = src_x % 8;

			if (byte & (1U << bit)) {
				*dst_row = color;
			}
			dst_row++;
		}
	}
}

void
pxl_blit_transformed(pxl_canvas_t *cnv, const pxl_buf_t *pb,
		pxl_rect_t pb_r, int x, int y, int scale, pxl_flip_t flip) {
	assert(cnv && cnv->pb);
	assert(pb && pb->data);
	assert(pb_r.w >= 0 && pb_r.h >= 0);
	assert(scale >= 1);

	/* Fast path: no transformation needed */
	if (scale == 1 && flip == PXL_FLIP_NONE) {
		pxl_blit_rect(cnv, pb, pb_r, x, y);
		return;
	}

	/* Extract flip flags */
	bool flip_h = (flip & PXL_FLIP_H) != 0;
	bool flip_v = (flip & PXL_FLIP_V) != 0;

	/* Apply canvas offset */
	x += cnv->offset_x;
	y += cnv->offset_y;

	/* Destination rectangle before clipping */
	pxl_rect_t dst_rect = {
		.x = x,
		.y = y,
		.w = pb_r.w * scale,
		.h = pb_r.h * scale
	};

	/* Clip against scissor */
	pxl_rect_t clipped;
	if (!pxl_clip_rect(dst_rect, cnv->scissor, &clipped)) {
		return;  /* Completely outside scissor */
	}

	/* Gather approach: iterate over destination pixels sequentially */
	for (int dst_y = clipped.y; dst_y < clipped.y + clipped.h; dst_y++) {
		pxl_t *dst_row = pxl_buf_ptr(cnv->pb, clipped.x, dst_y);

		for (int dst_x = clipped.x; dst_x < clipped.x + clipped.w; dst_x++) {
			/* Map destination pixel back to source */
			int rel_x = dst_x - x;
			int rel_y = dst_y - y;

			int src_x = flip_h ?
				pb_r.x + pb_r.w - 1 - rel_x / scale :
				pb_r.x + rel_x / scale;
			int src_y = flip_v ?
				pb_r.y + pb_r.h - 1 - rel_y / scale :
				pb_r.y + rel_y / scale;

			/* Sequential write to destination */
			*dst_row++ = *pxl_buf_ptr(pb, src_x, src_y);
		}
	}
}
