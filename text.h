#ifndef PXL_TEXT_H
#define PXL_TEXT_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "blit.h"
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

typedef struct {
    const pxl_font_t **fonts;      /* Array of fonts to try in order */
    size_t           font_count;   /* Number of fonts in array */
    int              tracking;     /* 0 = use first font's tracking */
    int              leading;      /* 0 = use first font's leading */
    int              tab_width;    /* Tab width in character spaces (default=4) */
    int              x, y;         /* Cursor position (before canvas offset) */
    int              line_start_x; /* (private) X position at start of current line (for \n, \r) */
} pxl_writer_t;

void
pxl_writer_init(pxl_writer_t *w, const pxl_font_t **fonts, size_t font_count);

static inline void
pxl_writer_set_cursor(pxl_writer_t *w, int x, int y) {
    assert(w);
    w->x = x;
    w->y = y;
    w->line_start_x = x;
}

static inline int
pxl_tab_advance(const pxl_writer_t *w) {
    assert(w && w->font_count > 0);
    return w->tab_width * (w->fonts[0]->bitmask.width + w->tracking);
}

/* UTF-8 decoder: returns bytes consumed (1-4), outputs Unicode codepoint.
 * On invalid sequences, returns 1 and outputs U+FFFD (REPLACEMENT CHARACTER).
 */
int
pxl_utf8_decode(const char *text, uint32_t *out_codepoint);

/* Measurement ------------------------------------------------------------- */

 /* Returns bounds for a single rune */
pxl_rect_t
pxl_rune_bounds(const pxl_writer_t *w, uint32_t rune);

/* Returns bounds for a text string */
pxl_rect_t
pxl_text_bounds(const pxl_writer_t *w, const char *txt);

/* Returns bounds for text truncated to `max_bytes` bytes.
 * Caller must ensure `max_bytes` does not split a UTF-8 rune.
 */
pxl_rect_t
pxl_text_bounds_n(const pxl_writer_t *w, const char *txt, size_t max_bytes);


void
pxl_draw_rune(pxl_canvas_t *cnv, pxl_writer_t *w, uint32_t rune);

void
pxl_draw_text(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt);

/* Draw text truncated to `max_bytes` bytes.
 * Caller must ensure `max_bytes` does not split a UTF-8 rune.
 */
void
pxl_draw_text_n(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt, size_t max_bytes);


/* Returns bounds for the first line of text (up to \n or \r or \r\n).
 */
pxl_rect_t
pxl_textline_bounds(const pxl_writer_t *w, const char *txt);

void
pxl_draw_textline(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt);

/* Returns pointer after first of text (up to \n or \r or \r\n)
 * or txt unchanged if none.
 * Example:
 *   const char *p = text;
 *   while (*p) {
 *       pxl_draw_textline(&cnv, &w, p);
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


/* Text bounds with scaling. scale must be >= 1. */
pxl_rect_t
pxl_rune_bounds_transformed(const pxl_writer_t *w, uint32_t rune, int scale);

pxl_rect_t
pxl_text_bounds_transformed(const pxl_writer_t *w, const char *txt, int scale);

void
pxl_draw_rune_transformed(pxl_canvas_t *cnv, pxl_writer_t *w, uint32_t rune, int scale, pxl_flip_t flip);

void
pxl_draw_text_transformed(pxl_canvas_t *cnv, pxl_writer_t *w, const char *txt, int scale, pxl_flip_t flip);

#endif /* PXL_TEXT_H */
