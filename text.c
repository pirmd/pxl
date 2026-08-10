#include <assert.h>

#include "draw.h"
#include "geom.h"
#include "text.h"

/* Font helper: resolve rune to actual rune and index, handling fallback.
 * Returns false if rune is out of range and no fallback is available.
 */
static bool
pxl_font_resolve_rune(const pxl_font_t *font, uint32_t rune, uint32_t *out_rune, int *out_idx) {
	if (rune < font->rune_start || rune > font->rune_end) {
		if (font->fallback_rune == 0) {
			return false;
		}
		assert(font->fallback_rune >= font->rune_start && font->fallback_rune <= font->rune_end);
		rune = font->fallback_rune;
	}
	*out_rune = rune;
	*out_idx = (int)(rune - font->rune_start);
	assert(*out_idx >= 0 && *out_idx <= (int)(font->rune_end - font->rune_start));
	return true;
}

/* Font helper: get glyph metrics for a given index.
 * If per-glyph arrays are NULL, uses bitmask.width for width and 0 for offsets.
 */
static void
pxl_font_glyph_metrics(const pxl_font_t *font, int idx,
                       int *w, int *h, int *advance,
                       int *offset_x, int *offset_y) {
	int glyph_w = (font->glyph_widths) ? font->glyph_widths[idx] : font->bitmask.width;
	if (w) *w = glyph_w;
	if (h) *h = font->glyph_height;
	if (advance) *advance = (font->glyph_advances) ? font->glyph_advances[idx] : glyph_w;
	if (offset_x) *offset_x = (font->glyph_offsets_x) ? font->glyph_offsets_x[idx] : 0;
	if (offset_y) *offset_y = (font->glyph_offsets_y) ? font->glyph_offsets_y[idx] : 0;
}

/* UTF-8 decoder: returns bytes consumed (1-4), outputs Unicode codepoint.
 * On invalid sequences, returns 1 and outputs U+FFFD (REPLACEMENT CHARACTER).
 * Validates: continuation bytes, overlong sequences, surrogates, and codepoint range.
 */
int
pxl_utf8_decode(const char *text, uint32_t *out_codepoint) {
	assert(text && out_codepoint);

	unsigned char c0 = (unsigned char)text[0];
	uint32_t cp;
	int len;

	/* ASCII (1 byte) */
	if (c0 < 0x80) {
		*out_codepoint = c0;
		return 1;
	}

	/* Continuation byte as first byte -> invalid */
	if ((c0 & 0xC0) == 0x80)
		goto invalid;

	/* Determine sequence length and extract leading bits */
	if (c0 < 0xE0) { len = 2; cp = c0 & 0x1F; }
	else if (c0 < 0xF0) { len = 3; cp = c0 & 0x0F; }
	else if (c0 < 0xF8) { len = 4; cp = c0 & 0x07; }
	else goto invalid;

	/* Validate continuation bytes and decode */
	for (int i = 1; i < len; i++) {
		unsigned char c = (unsigned char)text[i];
		if (c == '\0' || (c & 0xC0) != 0x80)
			goto invalid;
		cp = (cp << 6) | (c & 0x3F);
	}

	/* Validate codepoint ranges */
	if ((len == 2 && cp < 0x80) ||
	    (len == 3 && (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF))) ||
	    (len == 4 && (cp < 0x10000 || cp > 0x10FFFF)))
		goto invalid;

	*out_codepoint = cp;
	return len;

invalid:
	*out_codepoint = 0xFFFD;
	return 1;
}

void
pxl_writer_init(pxl_writer_t *w, pxl_canvas_t *cnv, const pxl_font_t *font) {
	assert(w && cnv);
	assert(font && font->bitmask.data && font->bitmask.width > 0 &&
	       font->rune_start <= font->rune_end && font->glyph_height > 0);

	w->cnv       = cnv;
	w->font      = font;
	w->tracking  = font->tracking;
	w->leading   = font->leading;
	w->tab_width = 4;

	pxl_writer_set_cursor(w, 0, 0);
}

void
pxl_draw_rune(pxl_writer_t *w, uint32_t rune) {
	assert(w && w->cnv && w->font);

	const pxl_font_t *font = w->font;
	const int tracking = w->tracking ? w->tracking : font->tracking;

	switch (rune) {
	case '\n':
		w->x = w->line_start_x;
		w->y += w->leading;
		return;

	case '\r':
		w->x = w->line_start_x;
		return;

	case '\t':
		w->x += tracking * w->tab_width;
		return;
	}

	uint32_t actual_rune;
	int idx;
	if (!pxl_font_resolve_rune(font, rune, &actual_rune, &idx)) {
		return;
	}

	int glyph_w, glyph_h, advance, offset_x, offset_y;
	pxl_font_glyph_metrics(font, idx, &glyph_w, &glyph_h, &advance, &offset_x, &offset_y);

	assert((idx + 1) * glyph_h <= font->bitmask.height);
	assert(glyph_w <= font->bitmask.width);

	pxl_draw_bitmask(w->cnv, &font->bitmask,
		(pxl_rect_t){.y = idx * font->glyph_height, .w = glyph_w, .h = glyph_h},
		w->x + offset_x, w->y + offset_y);

	w->x += advance + tracking;
}

void
pxl_draw_text(pxl_writer_t *w, const char *txt) {
	assert(w && w->cnv && w->font);
	assert(txt);

	uint32_t codepoint;
	while (*txt) {
		txt += pxl_utf8_decode(txt, &codepoint);
		pxl_draw_rune(w, codepoint);
	}
}

pxl_rect_t
pxl_text_bounds(const pxl_writer_t *w, const char *txt) {
	assert(w && w->font);
	assert(txt);

	const pxl_font_t *font = w->font;
	const int tracking = w->tracking ? w->tracking : font->tracking;
	const int leading = w->leading ? w->leading : font->leading;
	const int glyph_height = font->glyph_height;

	pxl_rect_t b = {0};
	int width = 0;
	int newline_count = 0;

	uint32_t codepoint;
	while (*txt) {
		txt += pxl_utf8_decode(txt, &codepoint);
		uint32_t rune = codepoint;

		switch (rune) {
			case '\n':
				if (width > b.w) b.w = width;
				width = 0;
				newline_count++;
				continue;

			case '\r':
				if (width > b.w) b.w = width;
				width = 0;
				continue;

			case '\t':
				width += tracking * w->tab_width;
				continue;
		}

		uint32_t actual_rune;
		int idx;
		if (!pxl_font_resolve_rune(font, rune, &actual_rune, &idx)) {
			continue;
		}

		int glyph_w, advance;
		pxl_font_glyph_metrics(font, idx, &glyph_w, NULL, &advance, NULL, NULL);

		width += advance + tracking;
	}

	if (width > b.w) b.w = width;

	/* Calculate height: each \n creates a new line.
	 * Total lines = newline_count + 1 (if any content/newlines exist).
	 * Each line contributes glyph_height, each \n contributes leading.
	 */
	int total_lines = (newline_count == 0 && width == 0) ? 0 : newline_count + 1;
	b.h = total_lines * glyph_height + newline_count * leading;

	return b;
}

pxl_rect_t
pxl_rune_bounds(const pxl_writer_t *w, uint32_t rune) {
	assert(w && w->font);

	const pxl_font_t *font = w->font;

	/* Control characters return zero bounds */
	switch (rune) {
		case '\n':
		case '\r':
		case '\t':
			return (pxl_rect_t){0};
	}

	uint32_t actual_rune;
	int idx;
	if (!pxl_font_resolve_rune(font, rune, &actual_rune, &idx)) {
		return (pxl_rect_t){0};
	}

	int glyph_w, glyph_h;
	pxl_font_glyph_metrics(font, idx, &glyph_w, &glyph_h, NULL, NULL, NULL);

	return (pxl_rect_t){0, 0, glyph_w, glyph_h};
}
