#ifndef PXL_TEXT_H
#define PXL_TEXT_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "bitmask.h"
#include "canvas.h"
#include "geom.h"

/* Font: bitmask + metadata. Characters stored sequentially by rune.
 * Optional per-glyph arrays enable variable-width fonts (NULL = monospace).
 * Covers a single contiguous rune range [rune_start, rune_end].
 */
typedef struct {
	pxl_bitmask_t  bitmask;         /* Bitmask data (LSB=leftmost) */

	uint32_t       rune_start;      /* First rune in the font */
	uint32_t       rune_end;        /* Last rune in the font (inclusive) */
	uint32_t       fallback_rune;   /* Fallback character (0 = skip) */
	int            tracking;        /* Extra horizontal space (pixels, added to glyph advance) */
	int            leading;         /* Vertical line spacing (pixels, baseline to baseline, excludes glyph_height) */

	int            glyph_height;    /* Glyph height (pixels) */
	const uint8_t *glyph_widths;    /* Per-glyph widths (NULL = use bitmask.width) */
	const uint8_t *glyph_advances;  /* Per-glyph advances (NULL = use glyph_widths or bitmask.width) */
	const int8_t  *glyph_offsets_x; /* Per-glyph X offsets (NULL = 0) */
	const int8_t  *glyph_offsets_y; /* Per-glyph Y offsets (NULL = 0) */
} pxl_font_t;

/* Writer: cursor, fonts, and spacing for text rendering.
 * Control chars: \n (newline: y += leading), \r (return: x=0), \t (tab).
 * Note: User must keep cursor within bounds; draw functions respect scissor.
 */
typedef struct {
	const pxl_font_t **fonts;      /* Array of fonts to try in order */
	size_t           font_count;    /* Number of fonts in array */
	int              tracking;     /* 0 = use first font's tracking */
	int              leading;      /* 0 = use first font's leading */
	int              tab_width;    /* Tab width in character spaces (default=4) */
	int              x, y;         /* Cursor position (before canvas offset) */
	int              line_start_x; /* (private) X position at start of current line (for \n, \r) */
} pxl_writer_t;

/* Initialization */
void
pxl_writer_init(pxl_writer_t *w, const pxl_font_t **fonts, size_t font_count);

static inline void
pxl_writer_set_cursor(pxl_writer_t *w, int x, int y) {
	assert(w);
	w->x = x;
	w->y = y;
	w->line_start_x = x;
}

/* UTF-8 decoder: returns bytes consumed (1-4), outputs Unicode codepoint.
 * On invalid sequences, returns 1 and outputs U+FFFD (REPLACEMENT CHARACTER).
 */
int
pxl_utf8_decode(const char *text, uint32_t *out_codepoint);

/* Alignment */
typedef enum {
	PXL_ALIGN_LEFT,    /* Align to left edge */
	PXL_ALIGN_CENTER,  /* Align to center */
	PXL_ALIGN_RIGHT,   /* Align to right edge */
	/* Vertical alignment aliases */
	PXL_ALIGN_TOP = PXL_ALIGN_LEFT,
	PXL_ALIGN_MIDDLE = PXL_ALIGN_CENTER,
	PXL_ALIGN_BOTTOM = PXL_ALIGN_RIGHT,
} pxl_align_t;

/* Calculate aligned x position for text of width `text_w` within a container.
 * `x0` is the container's left edge, `container_w` is its width.
 * Note: If text_w > container_w, text will overflow. Use canvas scissor to clip.
 */
int
pxl_align_x(int x0, int container_w, int text_w, pxl_align_t align);

/* Calculate aligned y position for text of height `text_h` within a container.
 * `y0` is the container's top edge, `container_h` is its height.
 * Note: If text_h > container_h, text will overflow. Use canvas scissor to clip.
 */
int
pxl_align_y(int y0, int container_h, int text_h, pxl_align_t align);

/* Measurement */
pxl_rect_t
pxl_rune_bounds(const pxl_writer_t *w, uint32_t rune); /* Returns bounds for a single rune */

pxl_rect_t
pxl_text_bounds(const pxl_writer_t *w, const char *txt); /* Returns bounds for a text string */

/* Returns bounds for text truncated to `max_bytes` bytes.
 * Caller must ensure `max_bytes` does not split a UTF-8 rune.
 */
pxl_rect_t
pxl_text_bounds_n(const pxl_writer_t *w, const char *txt, size_t max_bytes);

/* Returns bounds for the first line (up to \n or \r).
 * Handles \r\n as a single line break.
 * Use pxl_next_textline() to get the next line start.
 */
pxl_rect_t
pxl_textline_bounds(const pxl_writer_t *w, const char *txt);

/* Drawing */
/* Draw a single rune at current writer cursor. Handles \n, \r, \t. */
void
pxl_draw_rune(pxl_canvas_t *cnv, pxl_writer_t *w, uint32_t rune);

/* Draw text string at current writer cursor. Advances cursor. */
void
pxl_draw_text(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt);

/* Draw text truncated to `max_bytes` bytes.
 * Caller must ensure `max_bytes` does not split a UTF-8 rune.
 */
void
pxl_draw_text_n(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt, size_t max_bytes);

/* Draws the first line (up to \n or \r).
 * Advances writer cursor: on \n, moves to next line (y += leading);
 * on \r, resets x to line_start_x (no y change).
 * Handles \r\n as \n.
 */
void
pxl_draw_textline(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt);

/* Returns pointer after first \n or \r in txt (or txt if none, never NULL).
 * Handles \r\n as a single line break.
 *
 * Example:
 *   const char *p = text;
 *   while (*p) {
 *       pxl_draw_textline(&cnv, &w, p);
 *       pxl_writer_set_cursor(&w, 0, w.y);
 *       p = pxl_next_textline(p);
 *   }
 */
static inline const char *
pxl_next_textline(const char *txt) {
	if (!txt) return txt;
	while (*txt) {
		if (*txt == '\n') return txt + 1;
		if (*txt == '\r') {
			/* Handle \r\n as single line break */
			if (txt[1] == '\n') return txt + 2;
			return txt + 1;
		}
		txt++;
	}
	return txt; /* Points to '\0' at end of string */
}

#endif /* PXL_TEXT_H */
