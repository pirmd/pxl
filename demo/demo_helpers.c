#include "demo_helpers.h"
#include <assert.h>

/* PRNG state definition */
uint32_t demo_rng_state = 0;

/* =========================================================================
 * Scaled drawing utilities
 * ========================================================================= */

void
demo_draw_bitmask_scaled(pxl_canvas_t *cnv, int scale,
                    const pxl_bitmask_t *bm, pxl_rect_t bm_r,
                    int cnv_x, int cnv_y) {
	assert(cnv && cnv->pb);
	assert(bm && bm->data);
	assert(scale > 0);
	assert(bm_r.w >= 0 && bm_r.h >= 0);

	/* Apply canvas offset */
	cnv_x += cnv->offset_x;
	cnv_y += cnv->offset_y;

	/* Destination rectangle before clipping */
	pxl_rect_t dst_rect = {
		.x = cnv_x,
		.y = cnv_y,
		.w = bm_r.w * scale,
		.h = bm_r.h * scale
	};

	/* Clip against scissor */
	pxl_rect_t clipped;
	if (!pxl_clip_rect(dst_rect, cnv->scissor, &clipped)) {
		return;  /* Completely outside scissor */
	}

	uint32_t color = cnv->color;

	/* Calculate source offsets in bitmask (in glyph coordinates) */
	int src_x_start = (clipped.x - cnv_x) / scale;
	int src_y_start = (clipped.y - cnv_y) / scale;
	int src_x_end = ((clipped.x + clipped.w + scale - 1) - cnv_x) / scale;
	int src_y_end = ((clipped.y + clipped.h + scale - 1) - cnv_y) / scale;

	/* Clamp to bitmask bounds */
	if (src_x_start < 0) src_x_start = 0;
	if (src_y_start < 0) src_y_start = 0;
	if (src_x_end > bm_r.w) src_x_end = bm_r.w;
	if (src_y_end > bm_r.h) src_y_end = bm_r.h;

	/* Iterate over visible bitmask region */
	for (int gy = src_y_start; gy < src_y_end; gy++) {
		for (int gx = src_x_start; gx < src_x_end; gx++) {
			size_t byte_idx = (size_t)(bm_r.y + gy) * (size_t)bm->stride + (size_t)(bm_r.x + gx) / 8u;
			uint8_t byte = bm->data[byte_idx];
			int bit = (bm_r.x + gx) % 8;
			if (byte & (1 << bit)) {
				/* Calculate the destination pixel area for this bit */
				int dst_x0 = cnv_x + gx * scale;
				int dst_y0 = cnv_y + gy * scale;
				int dst_x1 = dst_x0 + scale;
				int dst_y1 = dst_y0 + scale;

				/* Clip to the visible region */
				int px_x0 = dst_x0 > clipped.x ? dst_x0 : clipped.x;
				int px_y0 = dst_y0 > clipped.y ? dst_y0 : clipped.y;
				int px_x1 = dst_x1 < clipped.x + clipped.w ? dst_x1 : clipped.x + clipped.w;
				int px_y1 = dst_y1 < clipped.y + clipped.h ? dst_y1 : clipped.y + clipped.h;

				/* Draw the visible portion */
				for (int py = px_y0; py < px_y1; py++) {
					for (int px = px_x0; px < px_x1; px++) {
						*pxl_buf_ptr(cnv->pb, px, py) = color;
					}
				}
			}
		}
	}
}

pxl_rect_t
demo_text_bounds_scaled(const pxl_font_t *font, const char *str, int scale) {
	int max_w = 0;
	int total_w = 0;
	int glyph_h = font->glyph_height;
	for (int i = 0; str[i]; i++) {
		uint32_t rune = (uint8_t)str[i];
		int rune_idx = (int)(rune - font->rune_start);
		if (rune_idx >= 0 && rune_idx <= (int)(font->rune_end - font->rune_start)) {
			int gw = font->glyph_widths ? font->glyph_widths[rune_idx] : font->bitmask.width;
			total_w += gw * scale;
			if (gw * scale > max_w) max_w = gw * scale;
		}
	}
	return (pxl_rect_t){ .w = total_w, .h = glyph_h * scale };
}

void
demo_draw_text_scaled(pxl_canvas_t *cnv, const pxl_font_t *font,
                    const char *str, int scale, int x, int y) {
	assert(cnv && cnv->pb);
	assert(font);
	assert(str);
	assert(scale > 0);

	int glyph_w = font->bitmask.width;
	int glyph_h = font->glyph_height;
	pxl_rect_t bm_r = { .x = 0, .y = 0, .w = glyph_w, .h = glyph_h };
	int cur_x = x;
	for (int i = 0; str[i]; i++) {
		uint32_t rune = (uint8_t)str[i];
		int rune_idx = (int)(rune - font->rune_start);
		if (rune_idx < 0 || rune_idx > (int)(font->rune_end - font->rune_start)) continue;
		bm_r.y = rune_idx * glyph_h;
		int gw = font->glyph_widths ? font->glyph_widths[rune_idx] : glyph_w;
		bm_r.w = gw;
		demo_draw_bitmask_scaled(cnv, scale, &font->bitmask, bm_r, cur_x, y);
		cur_x += gw * scale;
	}
}

/* =========================================================================
 * Update FPS counter
 * ========================================================================= */

void
demo_update_fps(double now, int *current_fps) {
	assert(current_fps != NULL);
	static double t0 = 0;
	static int n = 0;
	if (t0 == 0) {
		t0 = now;
		return;
	}
	n++;
	if (now - t0 >= 1.0) {
		*current_fps = (int)((float)n / (float)(now - t0));
		n = 0;
		t0 = now;
	}
}
