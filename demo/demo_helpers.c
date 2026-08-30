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
	uint32_t color = cnv->color;
	for (int gy = 0; gy < bm_r.h; gy++) {
		for (int gx = 0; gx < bm_r.w; gx++) {
			size_t byte_idx = (size_t)(bm_r.y + gy) * (size_t)bm->stride + (size_t)(bm_r.x + gx) / 8u;
			uint8_t byte = bm->data[byte_idx];
			int bit = (bm_r.x + gx) % 8;
			if (byte & (1 << bit)) {
				for (int sy = 0; sy < scale; sy++) {
					for (int sx = 0; sx < scale; sx++) {
						*pxl_buf_ptr(cnv->pb, cnv_x + gx * scale + sx, cnv_y + gy * scale + sy) = color;
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
