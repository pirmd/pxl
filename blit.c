#include <assert.h>
#include <stdint.h>  /* for uint8_t */
#include <string.h>

#include "bitmask.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Blit a rectangle from a pixel buffer to the canvas at (cnv_x, cnv_y).
 * Caller must ensure pb_r is within pb bounds (asserted).
 */
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
        memcpy(cnv_row, pb_row, (size_t)dst_rect.w * sizeof(pxl_t));
        pb_row += pb_stride;
        cnv_row += cnv_stride;
    }
}

/* Draw a region from bitmask to canvas at (cnv_x, cnv_y).
 * Pixels where bitmask bit is 1 are drawn with canvas color.
 * Caller must ensure bm_r is within bm bounds (asserted).
 */
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
				if (m & (1U << (leading_bit_off + (unsigned)bit))) {
					dst[i + bit] = color;
				}
			}
			i += bits_to_do;
			bit_offset += (size_t)bits_to_do;
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
