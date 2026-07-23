#include <string.h>

#include "buf.h"
#include "canvas.h"
#include "geom.h"
#include "text.h"

#include "stest/stest.h"

#define COLOR_WHITE  0xFFFFFFFFU  /* Opaque white */

/* Test font ----------------------------------------------------------------- */

/* Simple 5x5 monospace font for characters 'A' (65), 'B' (66), 'C' (67), 'D' (68, used as fallback) */
static const uint8_t test_font_data[20] = {
	/* 'A' at row 0: 5x5 */
	0x07, /* 0b00000111 */
	0x15, /* 0b00010101 */
	0x15, /* 0b00010101 */
	0x1F, /* 0b00011111 */
	0x15, /* 0b00010101 */
	/* 'B' at row 5: 5x5 */
	0x1F, /* 0b00011111 */
	0x15, /* 0b00010101 */
	0x1F, /* 0b00011111 */
	0x15, /* 0b00010101 */
	0x1F, /* 0b00011111 */
	/* 'C' at row 10: 5x5 */
	0x0E, /* 0b00001110 */
	0x11, /* 0b00010001 */
	0x10, /* 0b00010000 */
	0x10, /* 0b00010000 */
	0x0E, /* 0b00001110 */
	/* 'D' (fallback) at row 15: 5x5 */
	0x1F, /* 0b00011111 */
	0x15, /* 0b00010101 */
	0x15, /* 0b00010101 */
	0x15, /* 0b00010101 */
	0x1F  /* 0b00011111 */
};

static const pxl_bitmask_t test_bitmask = {
	.data = test_font_data,
	.width = 5,
	.height = 20,
	.stride = 1
};

static const pxl_font_t test_font = {
	.bitmask = test_bitmask,
	.rune_start = 65,   /* 'A' */
	.rune_end = 68,     /* 'D' */
	.fallback_rune = 68,/* 'D' used as fallback */
	.tracking = 1,
	.leading = 6,
	.glyph_height = 5,
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL
};

/* Fixture ----------------------------------------------------------------- */

typedef struct {
	pxl_buf_t      pb;
	pxl_canvas_t   cnv;
	pxl_text_ctx_t text_ctx;
} fixture_t;

static void
pxl_buf_zero(pxl_buf_t *pb) {
	assert(pb && pb->data);
	memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pxl_t));
}

static bool
fixture_init(const st_ctx_t *ctx, fixture_t *f, int w, int h) {
	if (!st_check(ctx, pxl_buf_init(&f->pb, w, h) == PXL_SUCCESS, "pixbuf_init failed")) {
		return false;
	}
	if (!st_check(ctx, f->pb.data != NULL, "pixbuf data is NULL")) {
		return false;
	}

	pxl_canvas_init(&f->cnv, &f->pb);
	pxl_buf_zero(&f->pb);
	pxl_text_ctx_init(&f->text_ctx, &f->cnv, &test_font);

	return true;
}

static void
fixture_deinit(fixture_t *f) {
	pxl_buf_deinit(&f->pb);
}

/* Helpers ----------------------------------------------------------------- */

static inline bool
has_pixels_in_rect(const pxl_buf_t *pb, pxl_rect_t rect) {
	for (int y = rect.y; y < rect.y + rect.h; y++) {
		for (int x = rect.x; x < rect.x + rect.w; x++) {
			if (*pxl_buf_ptr(pb, x, y) != 0) return true;
		}
	}
	return false;
}

/* Test pxl_draw_rune ----------------------------------------------------------- */

static void
test_pxl_draw_rune_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_rune(&f.text_ctx, 'A');

	/* Check pixels in expected bounds */
	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 'A');
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'A' bounds at (%d,%d)", x, y);

	fixture_deinit(&f);
}

static void
test_pxl_draw_rune_fallback(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	/* Character 200 is out of range, should fallback to 'D' (68) */
	pxl_draw_rune(&f.text_ctx, 200);

	/* Check pixels in fallback bounds (D is at rune 68) */
	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 68);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in fallback bounds at (%d,%d)", x, y);

	fixture_deinit(&f);
}

static void
test_pxl_draw_rune_no_fallback(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Create a font with no fallback */
	pxl_font_t font_no_fallback = test_font;
	font_no_fallback.fallback_rune = 0;
	pxl_text_ctx_init(&f.text_ctx, &f.cnv, &font_no_fallback);

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x_pos = 5, y_pos = 5;
	pxl_text_set_cursor(&f.text_ctx, x_pos, y_pos);
	/* Character 200 should be skipped */
	pxl_draw_rune(&f.text_ctx, 200);

	/* No pixels should be drawn */
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			ST_CHECK(*pxl_buf_ptr(&f.pb, x, y) == 0,
			         "Pixel (%d,%d) modified by out-of-range rune with no fallback", x, y);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rune_with_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Set a scissor that clips part of the character */
	pxl_canvas_set_scissor(&f.cnv, 8, 5, 8, 8);
	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_rune(&f.text_ctx, 'A');

	/* Check that pixels appear in scissor area */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){8, 5, 8, 8}),
	         "No pixels drawn in scissor area");

	fixture_deinit(&f);
}

static void
test_pxl_draw_rune_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_offset(&f.cnv, 5, 5);
	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x = 0, y = 0;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_rune(&f.text_ctx, 'A');

	/* Should appear at (5,5) due to offset */
	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 'A');
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "Offset not applied to draw_rune");

	fixture_deinit(&f);
}

/* Test pxl_rune_bounds ------------------------------------------------------- */

static void
test_pxl_rune_bounds_basic(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 10, 10)) return;

	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 'A');
	ST_CHECK(bounds.w > 0, "rune bounds width should be > 0, got %d", bounds.w);
	ST_CHECK(bounds.h > 0, "rune bounds height should be > 0, got %d", bounds.h);

	fixture_deinit(&f);
}

static void
test_pxl_rune_bounds_control_chars(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 10, 10)) return;

	/* Control characters should return zero bounds */
	pxl_rect_t bounds_nl = pxl_rune_bounds(&f.text_ctx, '\n');
	ST_CHECK(bounds_nl.w == 0 && bounds_nl.h == 0,
	         "Newline bounds should be zero, got w=%d, h=%d", bounds_nl.w, bounds_nl.h);

	pxl_rect_t bounds_tab = pxl_rune_bounds(&f.text_ctx, '\t');
	ST_CHECK(bounds_tab.w == 0 && bounds_tab.h == 0,
	         "Tab bounds should be zero, got w=%d, h=%d", bounds_tab.w, bounds_tab.h);

	pxl_rect_t bounds_cr = pxl_rune_bounds(&f.text_ctx, '\r');
	ST_CHECK(bounds_cr.w == 0 && bounds_cr.h == 0,
	         "Carriage return bounds should be zero, got w=%d, h=%d", bounds_cr.w, bounds_cr.h);

	fixture_deinit(&f);
}

static void
test_pxl_rune_bounds_fallback(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 10, 10)) return;

	/* Out of range rune should use fallback bounds (D=68) */
	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 200);
	pxl_rect_t fallback_bounds = pxl_rune_bounds(&f.text_ctx, 68);
	ST_CHECK(bounds.w == fallback_bounds.w && bounds.h == fallback_bounds.h,
	         "Fallback bounds mismatch: got w=%d,h=%d, expected w=%d,h=%d",
	         bounds.w, bounds.h, fallback_bounds.w, fallback_bounds.h);

	fixture_deinit(&f);
}

static void
test_pxl_rune_bounds_no_fallback(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 10, 10)) return;

	/* Create a font with no fallback */
	pxl_font_t font_no_fallback = test_font;
	font_no_fallback.fallback_rune = 0;
	f.text_ctx.font = &font_no_fallback;

	/* Out of range rune with no fallback should return zero bounds */
	pxl_rect_t bounds = pxl_rune_bounds(&f.text_ctx, 200);
	ST_CHECK(bounds.w == 0 && bounds.h == 0,
	         "Out-of-range rune with no fallback should have zero bounds, got w=%d, h=%d",
	         bounds.w, bounds.h);

	fixture_deinit(&f);
}

/* Test pxl_draw_text ------------------------------------------------------------ */

static void
test_pxl_draw_text_basic(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "ABC";
	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_text(&f.text_ctx, text);

	/* Check pixels in expected bounds */
	pxl_rect_t bounds = pxl_text_bounds(&f.text_ctx, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'ABC' bounds at (%d,%d)", x, y);

	fixture_deinit(&f);
}

static void
test_pxl_draw_text_empty(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	int x_pos = 5, y_pos = 5;
	pxl_text_set_cursor(&f.text_ctx, x_pos, y_pos);
	pxl_draw_text(&f.text_ctx, "");

	/* No pixels should be modified */
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			ST_CHECK(*pxl_buf_ptr(&f.pb, x, y) == 0,
			         "Pixel (%d,%d) modified by empty string", x, y);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_text_with_newline(void) {
	int w = 40, h = 30;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "A\nB";
	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_text(&f.text_ctx, text);

	/* Check pixels in expected bounds */
	pxl_rect_t bounds = pxl_text_bounds(&f.text_ctx, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'A\\nB' bounds at (%d,%d)", x, y);

	fixture_deinit(&f);
}

static void
test_pxl_draw_text_with_tab(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "A\tB";
	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_text(&f.text_ctx, text);

	/* Check pixels in expected bounds */
	pxl_rect_t bounds = pxl_text_bounds(&f.text_ctx, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'A\\tB' bounds at (%d,%d)", x, y);

	fixture_deinit(&f);
}

static void
test_pxl_draw_text_with_scissor(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_scissor(&f.cnv, 10, 5, 20, 10);
	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "Hello";
	int x = 5, y = 5;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_text(&f.text_ctx, text);

	/* Check that pixels appear in scissor area (y=5 to y=15 covers glyph height) */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){10, 5, 20, 10}),
	         "No pixels drawn in scissor area");

	fixture_deinit(&f);
}

static void
test_pxl_draw_text_with_offset(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_offset(&f.cnv, 5, 5);
	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "PXL";
	int x = 0, y = 0;
	pxl_text_set_cursor(&f.text_ctx, x, y);
	pxl_draw_text(&f.text_ctx, text);

	/* Should appear at (5,5) due to offset */
	pxl_rect_t bounds = pxl_text_bounds(&f.text_ctx, text);
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "Offset not applied to draw_text");

	fixture_deinit(&f);
}

/* Test pxl_draw_text with proportional font ------------------------------------- */

static void
test_pxl_draw_text_proportional(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Create a proportional test font with 3 chars: 'A' (65), 'B' (66), 'C' (67) */
	static const uint8_t font_data[15] = {
		/* 'A': 3x5 at row 0 */
		0x07, /* 0b00000111 */
		0x15, /* 0b00010101 */
		0x15, /* 0b00010101 */
		0x1F, /* 0b00011111 */
		0x15, /* 0b00010101 */
		/* 'B': 5x5 at row 5 */
		0x1F, /* 0b00011111 */
		0x15, /* 0b00010101 */
		0x1F, /* 0b00011111 */
		0x15, /* 0b00010101 */
		0x1F, /* 0b00011111 */
		/* 'C': 4x5 at row 10 */
		0x0E, /* 0b00001110 */
		0x11, /* 0b00010001 */
		0x10, /* 0b00010000 */
		0x10, /* 0b00010000 */
		0x0E  /* 0b00001110 */
	};

	const pxl_bitmask_t bitmask = {
		.data = font_data,
		.width = 5,
		.height = 15,
		.stride = 1
	};

	/* Per-glyph metrics */
	static const uint8_t widths[3] = {3, 5, 4};
	static const uint8_t advances[3] = {4, 6, 5};
	static const int8_t offsets_x[3] = {0, 0, 0};
	static const int8_t offsets_y[3] = {0, 0, 0};

	pxl_font_t prop_font = {
		.bitmask = bitmask,
		.rune_start = 65, /* 'A' */
		.rune_end = 67,   /* 'C' */
		.fallback_rune = 0,
		.tracking = 0,
		.leading = 6,
		.glyph_height = 5,
		.glyph_widths = widths,
		.glyph_advances = advances,
		.glyph_offsets_x = offsets_x,
		.glyph_offsets_y = offsets_y
	};

	pxl_text_ctx_t prop_ctx;
	pxl_text_ctx_init(&prop_ctx, &f.cnv, &prop_font);

	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);

	const char *text = "ABC";
	int x_pos = 5, y_pos = 5;
	pxl_text_set_cursor(&prop_ctx, x_pos, y_pos);
	pxl_draw_text(&prop_ctx, text);

	/* Check pixels in expected bounds */
	pxl_rect_t bounds = pxl_text_bounds(&prop_ctx, text);
	pxl_rect_t expected = {x_pos, y_pos, bounds.w, bounds.h};
	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in proportional 'ABC' bounds at (%d,%d)", x_pos, y_pos);

	fixture_deinit(&f);
}

/* Test pxl_text_bounds ---------------------------------------------------------- */

static void
test_pxl_text_bounds_basic(void) {
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, 10, 10)) return;

	/* Test simple text */
	pxl_rect_t bounds = pxl_text_bounds(&f.text_ctx, "Hello");
	ST_CHECK(bounds.w > 0, "text bounds width should be > 0, got %d", bounds.w);
	ST_CHECK(bounds.h > 0, "text bounds height should be > 0, got %d", bounds.h);

	/* Test with newline */
	bounds = pxl_text_bounds(&f.text_ctx, "Hello\nWorld");
	ST_CHECK(bounds.h == f.text_ctx.font->leading * 2,
	         "text with newline should have height = 2 * leading, got %d, expected %d",
	         bounds.h, f.text_ctx.font->leading * 2);

	/* Test empty string */
	bounds = pxl_text_bounds(&f.text_ctx, "");
	ST_CHECK(bounds.w == 0, "empty text should have width 0, got %d", bounds.w);
	ST_CHECK(bounds.h == 0, "empty text should have height 0, got %d", bounds.h);

	/* Test with tab */
	bounds = pxl_text_bounds(&f.text_ctx, "A\tB");
	ST_CHECK(bounds.w > 0, "text with tab should have width > 0, got %d", bounds.w);

	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */

int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Rune bounds tests */
		ST_T(test_pxl_rune_bounds_basic),
		ST_T(test_pxl_rune_bounds_control_chars),
		ST_T(test_pxl_rune_bounds_fallback),
		ST_T(test_pxl_rune_bounds_no_fallback),

		/* Character tests */
		ST_T(test_pxl_draw_rune_basic),
		ST_T(test_pxl_draw_rune_fallback),
		ST_T(test_pxl_draw_rune_no_fallback),
		ST_T(test_pxl_draw_rune_with_scissor),
		ST_T(test_pxl_draw_rune_with_offset),

		/* String tests */
		ST_T(test_pxl_draw_text_basic),
		ST_T(test_pxl_draw_text_empty),
		ST_T(test_pxl_draw_text_with_newline),
		ST_T(test_pxl_draw_text_with_tab),
		ST_T(test_pxl_draw_text_with_scissor),
		ST_T(test_pxl_draw_text_with_offset),
		ST_T(test_pxl_draw_text_proportional),

		/* Bounds tests */
		ST_T(test_pxl_text_bounds_basic)
	);
}
