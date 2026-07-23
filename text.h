#ifndef PXL_TEXT_H
#define PXL_TEXT_H

#include "bitmask.h"
#include "canvas.h"

/* Font: bitmask (LSB) + metadata. Characters stored sequentially by rune.
 * Optional arrays enable proportional-like fonts (NULL = monospace).
 */
typedef struct {
	pxl_bitmask_t  bitmask;         /* Bitmask data (LSB=leftmost) */

	uint32_t       rune_start;      /* First rune in the font                 */
	uint32_t       rune_end;        /* Last rune in the font                  */
	uint32_t       fallback_rune;   /* Fallback character (0 = skip)               */
	int            tracking;        /* Space between glyphs (pixels)               */
	int            leading;         /* Line spacing (pixels, baseline to baseline) */

	int            glyph_height;    /* Glyph height (pixels)                                */
	const uint8_t *glyph_widths;    /* Per-glyph widths    (NULL = use bitmask.width)       */
	const uint8_t *glyph_advances;  /* Per-glyph advances  (NULL = glyph_widths + tracking) */
	const int8_t  *glyph_offsets_x; /* Per-glyph X offsets (NULL = 0)                       */
	const int8_t  *glyph_offsets_y; /* Per-glyph Y offsets (NULL = 0)                       */
} pxl_font_t;

/* Text writing context: writing state with cursor, font, and writing specific spacing. */
typedef struct {
	pxl_canvas_t     *cnv;          /* Target canvas (public: access offset/scissor/color) */
	const pxl_font_t *font;         /* Active font */
	int               tracking;     /* 0 = use font->tracking */
	int               leading;      /* 0 = use font->leading */
	int               tab_width;    /* 0 = use default (4) */
	int               x, y;         /* Cursor position (before canvas offset) */
	int               line_start_x; /* private: X position at start of current line (for \n, \r) */
} pxl_text_ctx_t;

/* Initialization */
void
pxl_text_ctx_init(pxl_text_ctx_t *ctx, pxl_canvas_t *cnv, const pxl_font_t *font);

static void
pxl_text_set_cursor(pxl_text_ctx_t *ctx, int x, int y) {
	assert(ctx);
	assert(ctx->cnv && ctx->cnv->pb);
	assert(x >= 0 && x < ctx->cnv->pb->width);
	assert(y >= 0 && y < ctx->cnv->pb->height);

	ctx->x = x;
	ctx->y = y;
	ctx->line_start_x = x;
}

/* Writing */
void
pxl_draw_rune(pxl_text_ctx_t *ctx, uint32_t rune);

void
pxl_draw_text(pxl_text_ctx_t *ctx, const char *txt);

/* Measurement */
pxl_rect_t
pxl_rune_bounds(const pxl_text_ctx_t *ctx, uint32_t rune);

pxl_rect_t
pxl_text_bounds(const pxl_text_ctx_t *ctx, const char *txt);

#endif /* PXL_TEXT_H */
