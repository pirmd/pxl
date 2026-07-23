#include <assert.h>

#include "draw.h"
#include "geom.h"
#include "text.h"

void
pxl_text_ctx_init(pxl_text_ctx_t *ctx, pxl_canvas_t *cnv, const pxl_font_t *font) {
	assert(ctx && cnv);
	assert(font && font->bitmask.data && font->rune_start <= font->rune_end);

	ctx->cnv       = cnv;
	ctx->font      = font;
	ctx->tracking  = font->tracking;
	ctx->leading   = font->leading;
	ctx->tab_width = 4;

	pxl_text_set_cursor(ctx, 0, 0);
}

void
pxl_draw_rune(pxl_text_ctx_t *ctx, uint32_t rune) {
	assert(ctx && ctx->cnv && ctx->font);

	const pxl_font_t *font = ctx->font;
	const int tracking = ctx->tracking ? ctx->tracking : font->tracking;

	switch (rune) {
	case '\n':
		ctx->x = ctx->line_start_x;
		ctx->y += ctx->leading;
		return;

	case '\r':
		ctx->x = ctx->line_start_x;
		return;

	case '\t':
		ctx->x += tracking * (ctx->tab_width ? ctx->tab_width : 4);
		return;
	}

	if (rune < font->rune_start || rune > font->rune_end) {
		if (font->fallback_rune == 0) {
			return;
		}
		
		assert(font->fallback_rune >= font->rune_start && font->fallback_rune <= font->rune_end);
		rune = font->fallback_rune;
	}

	const int idx = (int)(rune - font->rune_start);
	
	/* Glyph dimensions */
	pxl_rect_t glyph_r = {
	   	.y = idx * font->glyph_height,
	   	.w = (font->glyph_widths) ? font->glyph_widths[idx] : font->bitmask.width,
	    .h = font->glyph_height
	};

	assert(glyph_r.y + glyph_r.h <= font->bitmask.height);
	assert(glyph_r.w <= font->bitmask.width);

	const int offset_x = (font->glyph_offsets_x) ? font->glyph_offsets_x[idx] : 0;
	const int offset_y = (font->glyph_offsets_y) ? font->glyph_offsets_y[idx] : 0;

	pxl_draw_bitmask(ctx->cnv,
		&font->bitmask, glyph_r,
		ctx->x + offset_x, ctx->y + offset_y
	);

	const int advance = (font->glyph_advances) ? font->glyph_advances[idx] : glyph_r.w;
	ctx->x += advance + tracking;
}

void
pxl_draw_text(pxl_text_ctx_t *ctx, const char *txt) {
	assert(ctx && ctx->cnv && ctx->font);
	assert(txt);

	while (*txt) {
		pxl_draw_rune(ctx, (uint32_t)(unsigned char)*txt);
		++txt;
	}
}

pxl_rect_t
pxl_text_bounds(const pxl_text_ctx_t *ctx, const char *txt) {
	assert(ctx && ctx->font);
	assert(txt);

	const pxl_font_t *font = ctx->font;
	const int tracking = ctx->tracking ? ctx->tracking : font->tracking;
	const int leading = ctx->leading ? ctx->leading : font->leading;

	pxl_rect_t b = {0};
	int w = 0;

	while (*txt) {
		uint32_t rune = (uint32_t)(unsigned char)*txt++;

		switch (rune) {
			case '\n':
				if (w > b.w) b.w = w;
				w = 0;
				b.h += leading;
				continue;

			case '\r':
				if (w > b.w) b.w = w;
				w = 0;
				continue;

			case '\t':
				w += tracking * (ctx->tab_width ? ctx->tab_width : 4);
				continue;
		}

		if (rune < font->rune_start || rune > font->rune_end) {
			if (font->fallback_rune == 0) {
				continue;
			}

			assert(font->fallback_rune >= font->rune_start && font->fallback_rune <= font->rune_end);
			rune = font->fallback_rune;
		}

		const int idx = (int)(rune - font->rune_start);
		const int glyph_w = (font->glyph_widths) ? font->glyph_widths[idx] : font->bitmask.width;
		const int advance = (font->glyph_advances) ? font->glyph_advances[idx] : glyph_w;

		w += advance + tracking;
	}

	if (w > b.w) b.w = w;
	if (w > 0) b.h += leading;

	return b;
}

pxl_rect_t
pxl_rune_bounds(const pxl_text_ctx_t *ctx, uint32_t rune) {
	assert(ctx && ctx->font);

	const pxl_font_t *font = ctx->font;

	/* Control characters return zero bounds */
	switch (rune) {
		case '\n':
		case '\r':
		case '\t':
			return (pxl_rect_t){0};
	}

	/* Handle fallback */
	uint32_t actual_rune = rune;
	if (rune < font->rune_start || rune > font->rune_end) {
		if (font->fallback_rune == 0) {
			return (pxl_rect_t){0};
		}
		actual_rune = font->fallback_rune;
	}

	const int idx = (int)(actual_rune - font->rune_start);
	const int glyph_w = (font->glyph_widths) ? font->glyph_widths[idx] : font->bitmask.width;
	const int glyph_h = font->glyph_height;

	return (pxl_rect_t){0, 0, glyph_w, glyph_h};
}
