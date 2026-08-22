#include <string.h>

#include "test.h"
#include "buf.h"
#include "canvas.h"
#include "geom.h"
#include "text.h"

#define COLOR_WHITE  0xFFFFFFFFU
#define FIXTURE_W    40
#define FIXTURE_H    40

/* Test font - 5x5 monospace for 'A' (65), 'B' (66), 'C' (67), 'D' (68, fallback) */
static const uint8_t g_test_font_data[20] = {
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

static const pxl_bitmask_t g_test_bitmask = {
	.data = g_test_font_data,
	.width = 5,
	.height = 20,
	.stride = 1
};

static const pxl_font_t g_test_font = {
	.bitmask = g_test_bitmask,
	.rune_start = 65,   /* 'A' */
	.rune_end = 68,     /* 'D' */
	.fallback_rune = 68,/* 'D' */
	.tracking = 1,
	.leading = 6,
	.glyph_height = 5,
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL
};

/* Test fonts for cascade: each covers a single rune */
static const uint8_t g_font_a_data[5] = {0x07, 0x15, 0x15, 0x1F, 0x15};  /* 'A' */
static const pxl_bitmask_t g_font_a_bitmask = {.data = g_font_a_data, .width = 5, .height = 5, .stride = 1};
static const pxl_font_t g_font_a = {
	.bitmask = g_font_a_bitmask,
	.rune_start = 65,
	.rune_end = 65,
	.fallback_rune = 0,
	.tracking = 1,
	.leading = 6,
	.glyph_height = 5,
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL
};

static const uint8_t g_font_c_data[5] = {0x0E, 0x11, 0x10, 0x10, 0x0E};  /* 'C' */
static const pxl_bitmask_t g_font_c_bitmask = {.data = g_font_c_data, .width = 5, .height = 5, .stride = 1};
static const pxl_font_t g_font_c = {
	.bitmask = g_font_c_bitmask,
	.rune_start = 67,
	.rune_end = 67,
	.fallback_rune = 0,
	.tracking = 1,
	.leading = 6,
	.glyph_height = 5,
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL
};

static const uint8_t g_font_lowercase_a_data[5] = {0x10, 0x28, 0x10, 0x2A, 0x1C};  /* 'a' */
static const pxl_bitmask_t g_font_lowercase_a_bitmask = {.data = g_font_lowercase_a_data, .width = 5, .height = 5, .stride = 1};
static const pxl_font_t g_font_lowercase_a = {
	.bitmask = g_font_lowercase_a_bitmask,
	.rune_start = 97,
	.rune_end = 97,
	.fallback_rune = 0,
	.tracking = 1,
	.leading = 6,
	.glyph_height = 5,
	.glyph_widths = NULL,
	.glyph_advances = NULL,
	.glyph_offsets_x = NULL,
	.glyph_offsets_y = NULL
};

/* Fixture */
static pxl_buf_t g_pb;
static pxl_canvas_t g_cnv;
static pxl_writer_t g_w;
static pxl_t g_buf_data[FIXTURE_H][FIXTURE_W];

static void
setup_fixture(void) {
	g_pb.width = FIXTURE_W;
	g_pb.height = FIXTURE_H;
	g_pb.stride = FIXTURE_W;
	g_pb.data = &g_buf_data[0][0];
	pxl_canvas_init(&g_cnv, &g_pb);
	memset(g_buf_data, 0x00, sizeof(g_buf_data));
	const pxl_font_t *fonts[] = {&g_test_font};
	pxl_writer_init(&g_w, fonts, 1);
}

/* Helpers */
static bool
has_pixels_in_rect(pxl_rect_t rect) {
	for (int y = rect.y; y < rect.y + rect.h; y++) {
		for (int x = rect.x; x < rect.x + rect.w; x++) {
			if (g_buf_data[y][x] != 0) return true;
		}
	}
	return false;
}

static bool
buf_is_empty(void) {
	for (int y = 0; y < FIXTURE_H; y++) {
		for (int x = 0; x < FIXTURE_W; x++) {
			if (g_buf_data[y][x] != 0) return false;
		}
	}
	return true;
}

/* Tests for pxl_utf8_decode */

static void
test_pxl_utf8_decode_ascii(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("A", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 'A');
}

static void
test_pxl_utf8_decode_2byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xC2\xA9", &codepoint);
	ASSERT(len == 2);
	ASSERT(codepoint == 0xA9);
}

static void
test_pxl_utf8_decode_3byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xE2\x82\xAC", &codepoint);
	ASSERT(len == 3);
	ASSERT(codepoint == 0x20AC);
}

static void
test_pxl_utf8_decode_4byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xF0\x9F\x98\x80", &codepoint);
	ASSERT(len == 4);
	ASSERT(codepoint == 0x1F600);
}

static void
test_pxl_utf8_decode_invalid_byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xFF", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_continuation_as_first(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\x80", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_2byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xC0", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_3byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xE2\x82", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_4byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xF0\x9F\x98", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_invalid_continuation_byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xC0\x22", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_overlong_2byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xC0\x80", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_overlong_3byte(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xE0\x80\x81", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_surrogate(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xED\xA0\x80", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_above_10ffff(void) {
	uint32_t codepoint;
	int len = pxl_utf8_decode("\xF4\x90\x80\x80", &codepoint);
	ASSERT(len == 1);
	ASSERT(codepoint == 0xFFFD);
}

/* Tests for pxl_rune_bounds */

static void
test_pxl_rune_bounds_basic(void) {
	setup_fixture();
	pxl_rect_t bounds = pxl_rune_bounds(&g_w, 'A');
	ASSERT(bounds.w > 0);
	ASSERT(bounds.h > 0);
}

static void
test_pxl_rune_bounds_control_chars(void) {
	setup_fixture();
	pxl_rect_t bounds_nl = pxl_rune_bounds(&g_w, '\n');
	ASSERT(bounds_nl.w == 0 && bounds_nl.h == 0);

	pxl_rect_t bounds_tab = pxl_rune_bounds(&g_w, '\t');
	ASSERT(bounds_tab.w == 0 && bounds_tab.h == 0);

	pxl_rect_t bounds_cr = pxl_rune_bounds(&g_w, '\r');
	ASSERT(bounds_cr.w == 0 && bounds_cr.h == 0);
}

static void
test_pxl_rune_bounds_fallback(void) {
	setup_fixture();
	pxl_rect_t bounds = pxl_rune_bounds(&g_w, 200);
	pxl_rect_t fallback_bounds = pxl_rune_bounds(&g_w, 68);
	ASSERT(bounds.w == fallback_bounds.w && bounds.h == fallback_bounds.h);
}

static void
test_pxl_rune_bounds_no_fallback(void) {
	setup_fixture();
	pxl_font_t font_no_fallback = g_test_font;
	font_no_fallback.fallback_rune = 0;

	pxl_writer_t ctx;
	const pxl_font_t *fonts[] = {&font_no_fallback};
	pxl_writer_init(&ctx, fonts, 1);

	pxl_rect_t bounds = pxl_rune_bounds(&ctx, 200);
	ASSERT(bounds.w == font_no_fallback.bitmask.width &&
	       bounds.h == font_no_fallback.glyph_height);
}

/* Tests for pxl_draw_rune */

static void
test_pxl_draw_rune_basic(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_rune(&g_cnv, &g_w, 'A');

	pxl_rect_t bounds = pxl_rune_bounds(&g_w, 'A');
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_rune_fallback(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_rune(&g_cnv, &g_w, 200);

	pxl_rect_t bounds = pxl_rune_bounds(&g_w, 68);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_rune_no_fallback(void) {
	setup_fixture();
	pxl_font_t font_no_fallback = g_test_font;
	font_no_fallback.fallback_rune = 0;
	const pxl_font_t *fonts[] = {&font_no_fallback};
	pxl_writer_init(&g_w, fonts, 1);

	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	int x_before = g_w.x;
	pxl_draw_rune(&g_cnv, &g_w, 200);

	ASSERT(buf_is_empty());
	ASSERT(g_w.x > x_before); /* Cursor must advance even for missing rune */
}

static void
test_pxl_draw_rune_with_scissor(void) {
	setup_fixture();
	pxl_canvas_set_scissor(&g_cnv, 8, 5, 8, 8);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_rune(&g_cnv, &g_w, 'A');

	ASSERT(has_pixels_in_rect((pxl_rect_t){8, 5, 8, 8}));
}

static void
test_pxl_draw_rune_with_offset(void) {
	setup_fixture();
	pxl_canvas_set_offset(&g_cnv, 5, 5);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 0, y = 0;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_rune(&g_cnv, &g_w, 'A');

	pxl_rect_t bounds = pxl_rune_bounds(&g_w, 'A');
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

/* Tests for pxl_draw_text */

static void
test_pxl_draw_text_basic(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "ABC";
	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, text);

	pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_empty(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, "");

	ASSERT(buf_is_empty());
}

static void
test_pxl_draw_text_with_newline(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "A\nB";
	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, text);

	pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_with_tab(void) {
	setup_fixture();
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "A\tB";
	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, text);

	pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_with_scissor(void) {
	setup_fixture();
	pxl_canvas_set_scissor(&g_cnv, 10, 5, 20, 10);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "Hello";
	int x = 5, y = 5;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, text);

	ASSERT(has_pixels_in_rect((pxl_rect_t){10, 5, 20, 10}));
}

static void
test_pxl_draw_text_with_offset(void) {
	setup_fixture();
	pxl_canvas_set_offset(&g_cnv, 5, 5);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "PXL";
	int x = 0, y = 0;
	pxl_writer_set_cursor(&g_w, x, y);
	pxl_draw_text(&g_cnv, &g_w, text);

	pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_proportional(void) {
	setup_fixture();

	static const uint8_t font_data[15] = {
		0x07, 0x15, 0x15, 0x1F, 0x15,
		0x1F, 0x15, 0x1F, 0x15, 0x1F,
		0x0E, 0x11, 0x10, 0x10, 0x0E
	};

	const pxl_bitmask_t bitmask = {
		.data = font_data,
		.width = 5,
		.height = 15,
		.stride = 1
	};

	static const uint8_t widths[3] = {3, 5, 4};
	static const uint8_t advances[3] = {4, 6, 5};
	static const int8_t offsets_x[3] = {0, 0, 0};
	static const int8_t offsets_y[3] = {0, 0, 0};

	pxl_font_t prop_font = {
		.bitmask = bitmask,
		.rune_start = 65,
		.rune_end = 67,
		.fallback_rune = 0,
		.tracking = 0,
		.leading = 6,
		.glyph_height = 5,
		.glyph_widths = widths,
		.glyph_advances = advances,
		.glyph_offsets_x = offsets_x,
		.glyph_offsets_y = offsets_y
	};

	pxl_writer_t prop_ctx;
	const pxl_font_t *fonts[] = {&prop_font};
	pxl_writer_init(&prop_ctx, fonts, 1);

	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	const char *text = "ABC";
	int x = 5, y = 5;
	pxl_writer_set_cursor(&prop_ctx, x, y);
	pxl_draw_text(&g_cnv, &prop_ctx, text);

	pxl_rect_t bounds = pxl_text_bounds(&prop_ctx, text);
	pxl_rect_t expected = {x, y, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

/* Tests for pxl_text_bounds */

static void
test_pxl_text_bounds_basic(void) {
	setup_fixture();
	pxl_rect_t bounds = pxl_text_bounds(&g_w, "Hello");
	ASSERT(bounds.w > 0);
	ASSERT(bounds.h > 0);
}

static void
test_pxl_text_bounds_empty(void) {
	setup_fixture();
	pxl_rect_t bounds = pxl_text_bounds(&g_w, "");
	ASSERT(bounds.w == 0);
	ASSERT(bounds.h == 0);
}

static void
test_pxl_text_bounds_height(void) {
	setup_fixture();
	const int gh = g_w.fonts[0]->glyph_height;
	const int ld = g_w.fonts[0]->leading;

	pxl_rect_t bounds = pxl_text_bounds(&g_w, "A");
	ASSERT(bounds.h == gh);

	bounds = pxl_text_bounds(&g_w, "");
	ASSERT(bounds.h == 0);

	/* \n creates 2 lines: first line (empty), second line (empty) */
	bounds = pxl_text_bounds(&g_w, "\n");
	ASSERT(bounds.h == 2 * gh + ld);

	bounds = pxl_text_bounds(&g_w, "A\n");
	ASSERT(bounds.h == 2 * gh + ld);

	bounds = pxl_text_bounds(&g_w, "A\nB");
	ASSERT(bounds.h == 2 * gh + ld);

	bounds = pxl_text_bounds(&g_w, "A\nB\nC");
	ASSERT(bounds.h == 3 * gh + 2 * ld);

	/* Empty lines: consecutive newlines create visual empty lines */
	bounds = pxl_text_bounds(&g_w, "\n\n");
	ASSERT(bounds.h == 3 * gh + 2 * ld);

	bounds = pxl_text_bounds(&g_w, "A\n\nB");
	ASSERT(bounds.h == 3 * gh + 2 * ld);

	bounds = pxl_text_bounds(&g_w, "A\n\n");
	ASSERT(bounds.h == 3 * gh + 2 * ld);

	bounds = pxl_text_bounds(&g_w, "\nA");
	ASSERT(bounds.h == 2 * gh + ld);
}

static void
test_pxl_text_bounds_with_tab(void) {
	setup_fixture();
	pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\tB");
	ASSERT(bounds.w > 0);
}

static void
test_pxl_text_bounds_zero_tracking(void) {
	setup_fixture();
	pxl_font_t font_zero_tracking = g_test_font;
	font_zero_tracking.tracking = 0;
	const pxl_font_t *fonts[] = {&font_zero_tracking};
	pxl_writer_init(&g_w, fonts, 1);

	pxl_rect_t bounds_abc = pxl_text_bounds(&g_w, "ABC");
	pxl_rect_t bounds_a = pxl_text_bounds(&g_w, "A");

	ASSERT(bounds_abc.w > bounds_a.w);
	ASSERT(bounds_abc.w < bounds_a.w * 5);
}

static void
test_pxl_text_bounds_zero_leading(void) {
	setup_fixture();
	pxl_font_t font_zero_leading = g_test_font;
	font_zero_leading.leading = 0;
	const pxl_font_t *fonts[] = {&font_zero_leading};
	pxl_writer_init(&g_w, fonts, 1);

	pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\nB");
	const int gh = g_w.fonts[0]->glyph_height;

	ASSERT(bounds.h == 2 * gh);
}

/* Tests for font cascade */

static void
test_pxl_draw_rune_cascade_basic(void) {
	setup_fixture();
	pxl_writer_t cascade_w;
	const pxl_font_t *fonts[] = {&g_font_a, &g_font_c, &g_font_lowercase_a};
	pxl_writer_init(&cascade_w, fonts, 3);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	pxl_writer_set_cursor(&cascade_w, 5, 5);
	pxl_draw_rune(&g_cnv, &cascade_w, 'A');

	pxl_rect_t bounds = pxl_rune_bounds(&cascade_w, 'A');
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_rune_cascade_missing_rune(void) {
	setup_fixture();
	pxl_writer_t cascade_w;
	const pxl_font_t *fonts[] = {&g_font_a, &g_font_c, &g_font_lowercase_a};
	pxl_writer_init(&cascade_w, fonts, 3);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	pxl_writer_set_cursor(&cascade_w, 5, 5);
	pxl_draw_rune(&g_cnv, &cascade_w, 'B');  /* Not in any font, no fallback */

	/* Should not draw anything but cursor should advance */
	int x_before = 5;
	ASSERT(cascade_w.x > x_before);
	ASSERT(buf_is_empty());
}

static void
test_pxl_draw_rune_cascade_lowercase(void) {
	setup_fixture();
	pxl_writer_t cascade_w;
	const pxl_font_t *fonts[] = {&g_font_a, &g_font_c, &g_font_lowercase_a};
	pxl_writer_init(&cascade_w, fonts, 3);
	pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

	pxl_writer_set_cursor(&cascade_w, 5, 5);
	pxl_draw_rune(&g_cnv, &cascade_w, 'a');  /* In third font */

	pxl_rect_t bounds = pxl_rune_bounds(&cascade_w, 'a');
	pxl_rect_t expected = {5, 5, bounds.w, bounds.h};
	ASSERT(has_pixels_in_rect(expected));
}

static void
test_pxl_text_bounds_cascade(void) {
	setup_fixture();
	pxl_writer_t cascade_w;
	const pxl_font_t *fonts[] = {&g_font_a, &g_font_c, &g_font_lowercase_a};
	pxl_writer_init(&cascade_w, fonts, 3);

	pxl_rect_t bounds = pxl_text_bounds(&cascade_w, "AaC");
	ASSERT(bounds.w > 0);
	ASSERT(bounds.h == g_font_a.glyph_height);
}

/* Main */

int
main(void) {
	test_pxl_utf8_decode_ascii();
	test_pxl_utf8_decode_2byte();
	test_pxl_utf8_decode_3byte();
	test_pxl_utf8_decode_4byte();
	test_pxl_utf8_decode_invalid_byte();
	test_pxl_utf8_decode_continuation_as_first();
	test_pxl_utf8_decode_incomplete_2byte();
	test_pxl_utf8_decode_incomplete_3byte();
	test_pxl_utf8_decode_incomplete_4byte();
	test_pxl_utf8_decode_invalid_continuation_byte();
	test_pxl_utf8_decode_overlong_2byte();
	test_pxl_utf8_decode_overlong_3byte();
	test_pxl_utf8_decode_surrogate();
	test_pxl_utf8_decode_above_10ffff();

	test_pxl_rune_bounds_basic();
	test_pxl_rune_bounds_control_chars();
	test_pxl_rune_bounds_fallback();
	test_pxl_rune_bounds_no_fallback();

	test_pxl_draw_rune_basic();
	test_pxl_draw_rune_fallback();
	test_pxl_draw_rune_no_fallback();
	test_pxl_draw_rune_with_scissor();
	test_pxl_draw_rune_with_offset();

	test_pxl_draw_text_basic();
	test_pxl_draw_text_empty();
	test_pxl_draw_text_with_newline();
	test_pxl_draw_text_with_tab();
	test_pxl_draw_text_with_scissor();
	test_pxl_draw_text_with_offset();
	test_pxl_draw_text_proportional();

	test_pxl_text_bounds_basic();
	test_pxl_text_bounds_empty();
	test_pxl_text_bounds_height();
	test_pxl_text_bounds_with_tab();
	test_pxl_text_bounds_zero_tracking();
	test_pxl_text_bounds_zero_leading();

	/* Tests for font cascade */
	test_pxl_draw_rune_cascade_basic();
	test_pxl_draw_rune_cascade_missing_rune();
	test_pxl_draw_rune_cascade_lowercase();
	test_pxl_text_bounds_cascade();

	return 0;
}
