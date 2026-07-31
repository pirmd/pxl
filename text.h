#ifndef PXL_TEXT_H
#define PXL_TEXT_H

#include <stdint.h>

#include "bitmask.h"
#include "canvas.h"

/* Font: bitmask (LSB) + metadata. Characters stored sequentially by rune.
 * Optional arrays enable proportional-like fonts (NULL = monospace).
 * If per-glyph arrays are NULL, all glyphs use bitmask.width/height and 0 offsets.
 * Per-glyph arrays: glyph_widths and glyph_advances use uint8_t, limiting values to 255px.
 * Spacing: tracking is extra horizontal space added after each glyph (advance + tracking),
 *          leading is vertical space between baselines (excludes glyph_height).
 */
typedef struct {
	pxl_bitmask_t  bitmask;         /* Bitmask data (LSB=leftmost) */

	uint32_t       rune_start;      /* First rune in the font                 */
	uint32_t       rune_end;        /* Last rune in the font                  */
	uint32_t       fallback_rune;   /* Fallback character (0 = skip)         */
	int            tracking;        /* Extra horizontal space (pixels, added to glyph advance) */
	int            leading;         /* Vertical line spacing (pixels, baseline to baseline, excludes glyph_height) */

	int            glyph_height;    /* Glyph height (pixels)                */
	const uint8_t *glyph_widths;    /* Per-glyph widths (NULL = use bitmask.width) */
	const uint8_t *glyph_advances;  /* Per-glyph advances (NULL = use glyph_widths or bitmask.width) */
	const int8_t  *glyph_offsets_x; /* Per-glyph X offsets (NULL = 0)       */
	const int8_t  *glyph_offsets_y; /* Per-glyph Y offsets (NULL = 0)       */
} pxl_font_t;

/* Writer: writing state with cursor, font, and writing specific spacing.
 * Control characters:
 *   \n: newline (x=line_start_x, y += leading)
 *   \r: carriage return (x=line_start_x)
 *   \t: tab (x += tracking * tab_width)
 * After each glyph: x += glyph_advance + tracking
 */
typedef struct {
	pxl_canvas_t     *cnv;          /* Target canvas (public: access offset/scissor/color) */
	const pxl_font_t *font;         /* Active font */
	int               tracking;     /* 0 = use font->tracking */
	int               leading;      /* 0 = use font->leading */
	int               tab_width;    /* Tab width in character spaces (default=4, set by pxl_writer_init) */
	int               x, y;         /* Cursor position (before canvas offset) */
	int               line_start_x; /* (private) X position at start of current line (for \n, \r) */
} pxl_writer_t;

/* Initialization */
void
pxl_writer_init(pxl_writer_t *w, pxl_canvas_t *cnv, const pxl_font_t *font);

static inline void
pxl_writer_set_cursor(pxl_writer_t *w, int x, int y) {
	assert(w);
	assert(w->cnv && w->cnv->pb);
	assert(x >= 0 && x < w->cnv->pb->width);
	assert(y >= 0 && y < w->cnv->pb->height);

	w->x = x;
	w->y = y;
	w->line_start_x = x;
}

/* Writing */
void
pxl_draw_rune(pxl_writer_t *w, uint32_t rune);

void
pxl_draw_text(pxl_writer_t *w, const char *txt);

/* UTF-8 utilities */
int
pxl_utf8_decode(const char *text, uint32_t *out_codepoint);

/* Measurement */
pxl_rect_t
pxl_rune_bounds(const pxl_writer_t *w, uint32_t rune); /* Returns bounds for a single rune */

pxl_rect_t
pxl_text_bounds(const pxl_writer_t *w, const char *txt); /* Returns bounds for a text string */

#endif /* PXL_TEXT_H */
