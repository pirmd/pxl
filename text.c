#include <assert.h>

#include "draw.h"
#include "geom.h"
#include "text.h"

/* UTF-8 decoder: returns bytes consumed (1-4), outputs Unicode codepoint.
 * On invalid sequences, returns 1 and outputs fallback '?'.
 */
static int
pxl_utf8_decode(const char *text, uint32_t *out_codepoint) {
	unsigned char c = (unsigned char)text[0];

	/* ASCII (1 byte) */
	if (c < 0x80) {
		*out_codepoint = c;
		return 1;
	}
	/* 2-byte sequence */
	if ((c & 0xE0) == 0xC0) {
		*out_codepoint = ((c & 0x1F) << 6) | (text[1] & 0x3F);
		return 2;
	}
	/* 3-byte sequence */
	if ((c & 0xF0) == 0xE0) {
		*out_codepoint = ((c & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
		return 3;
	}
	/* 4-byte sequence */
	if ((c & 0xF8) == 0xF0) {
		*out_codepoint = ((c & 0x07) << 18) | ((text[1] & 0x3F) << 12) |
		                 ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
		return 4;
	}

	/* Invalid or desynced character */
	*out_codepoint = '?';
	return 1;
}

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

	uint32_t codepoint;
	while (*txt) {
		txt += pxl_utf8_decode(txt, &codepoint);
		pxl_draw_rune(ctx, codepoint);
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

	uint32_t codepoint;
	while (*txt) {
		txt += pxl_utf8_decode(txt, &codepoint);
		uint32_t rune = codepoint;

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
