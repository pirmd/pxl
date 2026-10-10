#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bitmask.h"
#include "blit.h"
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
    assert(len == 1);
    assert(codepoint == 'A');
}

static void
test_pxl_utf8_decode_2byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xC2\xA9", &codepoint);
    assert(len == 2);
    assert(codepoint == 0xA9);
}

static void
test_pxl_utf8_decode_3byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xE2\x82\xAC", &codepoint);
    assert(len == 3);
    assert(codepoint == 0x20AC);
}

static void
test_pxl_utf8_decode_4byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xF0\x9F\x98\x80", &codepoint);
    assert(len == 4);
    assert(codepoint == 0x1F600);
}

static void
test_pxl_utf8_decode_invalid_byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xFF", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_continuation_as_first(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\x80", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_2byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xC0", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_3byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xE2\x82", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_incomplete_4byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xF0\x9F\x98", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_invalid_continuation_byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xC0\x22", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_overlong_2byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xC0\x80", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_overlong_3byte(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xE0\x80\x81", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_surrogate(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xED\xA0\x80", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

static void
test_pxl_utf8_decode_above_10ffff(void) {
    uint32_t codepoint;
    int len = pxl_utf8_decode("\xF4\x90\x80\x80", &codepoint);
    assert(len == 1);
    assert(codepoint == 0xFFFD);
}

/* Tests for pxl_rune_bounds */

static void
test_pxl_rune_bounds_basic(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_rune_bounds(&g_w, 'A');
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_pxl_rune_bounds_control_chars(void) {
    setup_fixture();
    pxl_rect_t bounds_nl = pxl_rune_bounds(&g_w, '\n');
    assert(bounds_nl.w == 0 && bounds_nl.h == 0);

    pxl_rect_t bounds_tab = pxl_rune_bounds(&g_w, '\t');
    assert(bounds_tab.w == 0 && bounds_tab.h == 0);

    pxl_rect_t bounds_cr = pxl_rune_bounds(&g_w, '\r');
    assert(bounds_cr.w == 0 && bounds_cr.h == 0);
}

static void
test_pxl_rune_bounds_fallback(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_rune_bounds(&g_w, 200);
    pxl_rect_t fallback_bounds = pxl_rune_bounds(&g_w, 68);
    assert(bounds.w == fallback_bounds.w && bounds.h == fallback_bounds.h);
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
    assert(bounds.w == font_no_fallback.bitmask.width &&
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
    assert(has_pixels_in_rect(expected));
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
    assert(has_pixels_in_rect(expected));
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

    assert(buf_is_empty());
    assert(g_w.x > x_before); /* Cursor must advance even for missing rune */
}

static void
test_pxl_draw_rune_with_scissor(void) {
    setup_fixture();
    pxl_canvas_set_scissor(&g_cnv, 8, 5, 8, 8);
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_rune(&g_cnv, &g_w, 'A');

    assert(has_pixels_in_rect((pxl_rect_t){8, 5, 8, 8}));
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
    assert(has_pixels_in_rect(expected));
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
    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_empty(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text(&g_cnv, &g_w, "");

    assert(buf_is_empty());
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
    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_with_tab(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A\tB";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    int start_x = g_w.x;
    pxl_draw_text(&g_cnv, &g_w, text);

    /* Verify cursor advanced by exact amount */
    /* A = 5+1 = 6px, tab = 4*(5+1) = 24px, B = 5+1 = 6px */
    /* Total advance = 6 + 24 + 6 = 36px */
    assert(g_w.x == start_x + 36);

    pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
    pxl_rect_t expected = {x, y, bounds.w, bounds.h};
    assert(has_pixels_in_rect(expected));
    /* Bounds width must match actual cursor advance */
    assert(bounds.w == 36);
}

/* Tests for multi-line consistency */

static void
test_pxl_text_bounds_carriage_return_only(void) {
    setup_fixture();
    /* \r alone should NOT create a new line (height = 1 line) */
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\rB");
    assert(bounds.w > 0);
    assert(bounds.h == g_w.fonts[0]->glyph_height); /* Single line */
}

static void
test_pxl_text_bounds_crlf(void) {
    setup_fixture();
    /* \r\n should be treated as a single line break (2 lines total) */
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\r\nB");
    assert(bounds.w > 0);
    assert(bounds.h == g_w.fonts[0]->glyph_height + g_test_font.leading);
}

static void
test_pxl_text_bounds_double_newline(void) {
    setup_fixture();
    /* \n\n should create an empty line (3 lines total: A, empty, B) */
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\n\nB");
    assert(bounds.w > 0);
    assert(bounds.h == g_w.fonts[0]->glyph_height + 2 * g_test_font.leading);
}

static void
test_pxl_draw_text_with_carriage_return(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A\rB";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text(&g_cnv, &g_w, text);

    /* After \r, y should be unchanged (no line break), B overwrites A at start of line */
    assert(g_w.y == y);
}

/* Test the example from pxl_next_textline() documentation */
static void
test_pxl_next_textline_example(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A\nB\r\nC";
    pxl_writer_t w_local;
    const pxl_font_t *fonts[] = {&g_test_font};
    pxl_writer_init(&w_local, fonts, 1);
    pxl_writer_set_cursor(&w_local, 0, 0);

    const char *p = text;
    while (*p) {
        pxl_draw_textline(&g_cnv, &w_local, p);
        pxl_writer_set_cursor(&w_local, 0, w_local.y);
        p = pxl_next_textline(p);
    }

    /* Verify that something was drawn */
    assert(has_pixels_in_rect((pxl_rect_t){0, 0, 10, 10}));
}

static void
test_pxl_text_draw_consistency(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    /* Test that pxl_text_bounds and pxl_draw_text actually agree:
     * the bbox height must equal the real drawn extent, not just two
     * formulas that happen to look similar.
     * leading is the full line-to-line advance (pxl_draw_rune only adds
     * leading per \n), so pxl_text_bounds must count glyph_height once,
     * for the last line, plus newline_count * leading.
     * For "A\nB\r\nC": 3 lines (A, B, C), 2 newlines (\n and \n after \r)
     */
    const char *text = "A\nB\r\nC";
    pxl_rect_t bounds = pxl_text_bounds(&g_w, text);

    /* Draw the text */
    int start_y = 5;
    pxl_writer_set_cursor(&g_w, 0, start_y);
    pxl_draw_text(&g_cnv, &g_w, text);

    int glyph_height = g_w.fonts[0]->glyph_height;
    int leading = g_test_font.leading;
    int newline_count = 2; /* "A\nB\r\nC" has 2 newlines */
    int expected_height = glyph_height + newline_count * leading;
    assert(bounds.h == expected_height);

    /* Verify drawn y position: each \n advances by leading */
    int drawn_height = g_w.y - start_y;
    assert(drawn_height == newline_count * leading);

    /* The real cross-check: bounds.h must match where the last line
     * actually lands. drawn_height covers the advance to the top of the
     * last line; add that line's own glyph_height to get the full extent.
     * This is the assertion that would have caught draw/bounds drifting
     * apart, regardless of which side had the wrong formula. */
    assert(bounds.h == drawn_height + glyph_height);
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

    assert(has_pixels_in_rect((pxl_rect_t){10, 5, 20, 10}));
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
    assert(has_pixels_in_rect(expected));
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
    assert(has_pixels_in_rect(expected));
}

/* Tests for pxl_text_bounds */

static void
test_pxl_text_bounds_basic(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "Hello");
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_pxl_text_bounds_empty(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "");
    assert(bounds.w == 0);
    assert(bounds.h == 0);
}

static void
test_pxl_text_bounds_height(void) {
    setup_fixture();
    const int gh = g_w.fonts[0]->glyph_height;
    const int ld = g_w.fonts[0]->leading;

    pxl_rect_t bounds = pxl_text_bounds(&g_w, "A");
    assert(bounds.h == gh);

    bounds = pxl_text_bounds(&g_w, "");
    assert(bounds.h == 0);

    /* \n creates 2 lines: first line (empty), second line (empty).
     * leading is the full line-to-line advance (see pxl_draw_rune), so
     * only the last line contributes a full glyph_height. */
    bounds = pxl_text_bounds(&g_w, "\n");
    assert(bounds.h == gh + ld);

    bounds = pxl_text_bounds(&g_w, "A\n");
    assert(bounds.h == gh + ld);

    bounds = pxl_text_bounds(&g_w, "A\nB");
    assert(bounds.h == gh + ld);

    bounds = pxl_text_bounds(&g_w, "A\nB\nC");
    assert(bounds.h == gh + 2 * ld);

    /* Empty lines: consecutive newlines create visual empty lines */
    bounds = pxl_text_bounds(&g_w, "\n\n");
    assert(bounds.h == gh + 2 * ld);

    bounds = pxl_text_bounds(&g_w, "A\n\nB");
    assert(bounds.h == gh + 2 * ld);

    bounds = pxl_text_bounds(&g_w, "A\n\n");
    assert(bounds.h == gh + 2 * ld);

    bounds = pxl_text_bounds(&g_w, "\nA");
    assert(bounds.h == gh + ld);
}

static void
test_pxl_text_bounds_with_tab(void) {
    setup_fixture();
    /* Tab should advance by tab_width * (glyph_width + tracking) */
    int char_width = g_test_font.bitmask.width + g_test_font.tracking;
    int tab_advance = g_w.tab_width * char_width;
    assert(tab_advance == 24); /* 4 * (5 + 1) */
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "A\tB");
    /* Width of "A\tB" = width(A) + tab_advance + width(B) */
    /* width(A) = char_width = 6, tab_advance = 24, width(B) = 6 */
    /* Total = 6 + 24 + 6 = 36 */
    assert(bounds.w == 36);
}

static void
test_pxl_text_bounds_tab_exact_width(void) {
    setup_fixture();
    /* Single tab: should be tab_width * (glyph_width + tracking) */
    int expected_tab_width = g_w.tab_width * (g_test_font.bitmask.width + g_test_font.tracking);
    pxl_rect_t bounds = pxl_text_bounds(&g_w, "\t");
    assert(bounds.w == expected_tab_width);
    assert(bounds.h == 0); /* No glyph height for control char alone */

    /* Tab with character: "A\t" */
    /* = width(A) + tab_advance = (5+1) + 24 = 30 */
    bounds = pxl_text_bounds(&g_w, "A\t");
    assert(bounds.w == 30);

    /* Multiple tabs: "\t\t" */
    bounds = pxl_text_bounds(&g_w, "\t\t");
    assert(bounds.w == expected_tab_width * 2);
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

    assert(bounds_abc.w > bounds_a.w);
    assert(bounds_abc.w < bounds_a.w * 5);
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
    assert(bounds.h == gh);
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
    assert(has_pixels_in_rect(expected));
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
    assert(cascade_w.x > x_before);
    assert(buf_is_empty());
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
    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_text_bounds_cascade(void) {
    setup_fixture();
    pxl_writer_t cascade_w;
    const pxl_font_t *fonts[] = {&g_font_a, &g_font_c, &g_font_lowercase_a};
    pxl_writer_init(&cascade_w, fonts, 3);

    pxl_rect_t bounds = pxl_text_bounds(&cascade_w, "AaC");
    assert(bounds.w > 0);
    assert(bounds.h == g_font_a.glyph_height);
}

/* Tests for truncated text helpers */

static void
test_pxl_text_bounds_n_basic(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, "ABC", 10);
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_pxl_text_bounds_n_empty(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, "ABC", 0);
    assert(bounds.w == 0);
    assert(bounds.h == 0);
}

static void
test_pxl_text_bounds_n_partial(void) {
    setup_fixture();
    pxl_rect_t full_bounds = pxl_text_bounds(&g_w, "ABC");
    pxl_rect_t partial_bounds = pxl_text_bounds_n(&g_w, "ABC", 2);
    assert(partial_bounds.w <= full_bounds.w);
    assert(partial_bounds.h == full_bounds.h);
}

static void
test_pxl_text_bounds_n_with_newline(void) {
    setup_fixture();
    const char *text = "A\nBC";
    const char *first_line = text;
    const char *newline = strchr(text, '\n');
    size_t first_line_bytes = (size_t)(newline - first_line);

    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, text, first_line_bytes);
    assert(bounds.w > 0);
    assert(bounds.h == g_w.fonts[0]->glyph_height);
}

static void
test_pxl_draw_text_n_basic(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "ABC";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_n(&g_cnv, &g_w, text, 10);

    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, text, 10);
    pxl_rect_t expected = {x, y, bounds.w, bounds.h};
    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_n_empty(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_n(&g_cnv, &g_w, "ABC", 0);

    assert(buf_is_empty());
}

static void
test_pxl_draw_text_n_partial(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "ABC";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_n(&g_cnv, &g_w, text, 2);

    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, text, 2);
    pxl_rect_t expected = {x, y, bounds.w, bounds.h};
    assert(has_pixels_in_rect(expected));
}

/* Additional edge case tests for truncated text helpers */

static void
test_pxl_text_bounds_n_with_tab(void) {
    setup_fixture();
    /* Verify tab width calculation is consistent with pxl_text_bounds */
    pxl_rect_t bounds_full = pxl_text_bounds(&g_w, "A\tB");
    pxl_rect_t bounds_n = pxl_text_bounds_n(&g_w, "A\tB", 10);
    assert(bounds_n.w == bounds_full.w);
    assert(bounds_n.h == bounds_full.h);
    /* Both should be 36px wide (A=6, tab=24, B=6) */
    assert(bounds_n.w == 36);
}

static void
test_pxl_text_bounds_n_with_carriage_return(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, "A\rB", 10);
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_pxl_text_bounds_n_multiline(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds_n(&g_w, "A\nB\nC", 10);
    assert(bounds.w > 0);
    assert(bounds.h > g_w.fonts[0]->glyph_height); /* Multiple lines */
}

static void
test_pxl_draw_text_n_with_newline(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_n(&g_cnv, &g_w, "A\nB", 3); /* Only first line + newline */

    /* Should have drawn 'A' and newline moved cursor down */
    assert(has_pixels_in_rect((pxl_rect_t){x, y, 10, 10}));
}

static void
test_pxl_tab_consistency_across_functions(void) {
    setup_fixture();
    /* Verify all functions calculate tab width consistently */
    const char *text = "A\tB";
    int expected_width = 36; /* A(6) + tab(24) + B(6) */

    /* All bounds functions must agree */
    pxl_rect_t bounds = pxl_text_bounds(&g_w, text);
    pxl_rect_t bounds_n = pxl_text_bounds_n(&g_w, text, strlen(text));
    pxl_rect_t line_bounds = pxl_textline_bounds(&g_w, text);

    assert(bounds.w == expected_width);
    assert(bounds_n.w == expected_width);
    assert(line_bounds.w == expected_width);

    /* Verify draw functions advance cursor by same amount */
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);
    pxl_writer_set_cursor(&g_w, 0, 0);
    int start_x = g_w.x;
    pxl_draw_text(&g_cnv, &g_w, text);
    assert(g_w.x == start_x + expected_width);
}

/* Tests for line-based helpers */

static void
test_pxl_textline_bounds_basic(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_textline_bounds(&g_w, "ABC");
    assert(bounds.w > 0);
    assert(bounds.h == g_w.fonts[0]->glyph_height);
}

static void
test_pxl_textline_bounds_with_newline(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_textline_bounds(&g_w, "AB\nCD");
    assert(bounds.w > 0);
    assert(bounds.h == g_test_font.glyph_height);
    /* Use pxl_next_textline to get offset */
    const char *text = "AB\nCD";
    const char *next = pxl_next_textline(text);
    assert(next == text + 3); /* "AB\n" = 3 bytes */
}

static void
test_pxl_textline_bounds_empty(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_textline_bounds(&g_w, "");
    assert(bounds.w == 0);
    assert(bounds.h == 0);
}

static void
test_pxl_textline_bounds_with_tab(void) {
    setup_fixture();
    /* Tab in a single line: "A\tB" should be 36px wide */
    pxl_rect_t bounds = pxl_textline_bounds(&g_w, "A\tB");
    assert(bounds.w == 36);
    assert(bounds.h == g_w.fonts[0]->glyph_height);

    /* Tab at start: "\tA" */
    /* tab_advance = 4*(5+1) = 24, A = 5+1 = 6, total = 30 */
    bounds = pxl_textline_bounds(&g_w, "\tA");
    assert(bounds.w == 30);
}

static void
test_pxl_draw_textline_basic(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_textline(&g_cnv, &g_w, "ABC");

    pxl_rect_t bounds = pxl_textline_bounds(&g_w, "ABC");
    pxl_rect_t expected = {x, y, bounds.w, bounds.h};
    assert(has_pixels_in_rect(expected));
}

/* Tests for pxl_next_textline */

/* Tests for transformed text functions */

static void
test_pxl_text_bounds_transformed_basic(void) {
    setup_fixture();
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, "ABC", 1);
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_pxl_text_bounds_transformed_scale2(void) {
    setup_fixture();
    pxl_rect_t bounds1 = pxl_text_bounds_transformed(&g_w, "ABC", 1);
    pxl_rect_t bounds2 = pxl_text_bounds_transformed(&g_w, "ABC", 2);
    assert(bounds2.w == bounds1.w * 2);
    assert(bounds2.h == bounds1.h * 2);
}

static void
test_pxl_draw_text_transformed_basic(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "ABC";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_NONE);

    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, 1);
    pxl_rect_t expected = {x, y, bounds.w, bounds.h};
    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_text_transformed_scale2(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A";
    int x = 5, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    pxl_draw_text_transformed(&g_cnv, &g_w, text, 2, PXL_FLIP_NONE);

    /* With scale 2, the character should be twice as wide and tall */
    assert(has_pixels_in_rect((pxl_rect_t){x, y, 10, 10}));
}

static void
test_pxl_draw_text_transformed_with_flip(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A";
    int x = 10, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);

    /* Draw with horizontal flip */
    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_H);

    /* Just verify it draws something without crashing */
    assert(has_pixels_in_rect((pxl_rect_t){x, y, 10, 10}));
}

/* Tests for pxl_draw_rune_transformed with flip cursor behavior */

static void
test_pxl_draw_rune_transformed_flip_h_cursor_moves_left(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 20, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);
    int start_x = g_w.x;

    pxl_draw_rune_transformed(&g_cnv, &g_w, 'A', 1, PXL_FLIP_H);

    /* Cursor should move left (negative direction) */
    assert(g_w.x < start_x);
}

static void
test_pxl_draw_rune_transformed_flip_v_cursor_moves_up(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 20;
    pxl_writer_set_cursor(&g_w, x, y);
    int start_x = g_w.x;
    int start_y = g_w.y;

    pxl_draw_rune_transformed(&g_cnv, &g_w, 'A', 1, PXL_FLIP_V);

    /* With PXL_FLIP_V only, x advances normally (dx=1), y stays same (no \n) */
    assert(g_w.x > start_x); /* x advances normally */
    assert(g_w.y == start_y); /* y unchanged for single char */

    /* Now test newline with flip_v */
    pxl_writer_set_cursor(&g_w, x, y);
    start_y = g_w.y;
    pxl_draw_rune_transformed(&g_cnv, &g_w, '\n', 1, PXL_FLIP_V);
    assert(g_w.y < start_y); /* y moves up with flip_v */
}

static void
test_pxl_draw_rune_transformed_flip_h_newline_resets_to_left(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 20, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);

    pxl_draw_rune_transformed(&g_cnv, &g_w, '\n', 1, PXL_FLIP_H);

    /* After newline with flip_h, cursor should be at line_start_x (right side) */
    assert(g_w.x == g_w.line_start_x);
    assert(g_w.y > y); /* y advances normally (dy not applied to \n y-movement) */
}

static void
test_pxl_draw_rune_transformed_flip_h_newline_preserves_x(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    /* Test that \n with PXL_FLIP_H preserves X position for next line start */
    int x = 20, y = 5;
    pxl_writer_set_cursor(&g_w, x, y);

    /* Draw a character to move cursor left */
    pxl_draw_rune_transformed(&g_cnv, &g_w, 'A', 1, PXL_FLIP_H);
    int x_after_A = g_w.x;
    assert(x_after_A < x); /* Cursor moved left */

    /* Newline should preserve current X as line_start_x */
    pxl_draw_rune_transformed(&g_cnv, &g_w, '\n', 1, PXL_FLIP_H);
    assert(g_w.x == x_after_A); /* X unchanged by \n in flip_h mode */
    assert(g_w.y > y); /* Y advanced */
    assert(g_w.line_start_x == x_after_A); /* line_start_x updated to preserved X */
}

static void
test_pxl_draw_text_transformed_flip_h_reversed_order(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "ABC";
    int start_x = 20, start_y = 5;
    pxl_writer_set_cursor(&g_w, start_x, start_y);

    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_H);

    /* Text should be drawn right-to-left, final cursor position should be at start */
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, 1);
    assert(g_w.x == start_x); /* Cursor ended up at original start position */
    assert(g_w.y == start_y);
    /* Pixels should be drawn from start_x to start_x + bounds.w */
    assert(has_pixels_in_rect((pxl_rect_t){start_x, start_y, bounds.w, bounds.h}));
}

static void
test_pxl_draw_text_transformed_flip_v_reversed_lines(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A\nB";
    int start_x = 5, start_y = 20;
    pxl_writer_set_cursor(&g_w, start_x, start_y);

    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_V);

    /* Text should be drawn bottom-to-top */
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, 1);
    /* Starting y is start_y + bounds.h, after drawing two lines with flip_v,
     * final y should be start_y + bounds.h - leading (after \n) - advance_B */
    /* But x resets to line_start_x after \n, then advances for B */
    int expected_y = start_y + bounds.h - g_test_font.leading;
    assert(g_w.y == expected_y);
    /* x should be at line_start_x + advance_B + tracking */
    assert(g_w.x > start_x);
    /* Pixels should be drawn from start_y to start_y + bounds.h */
    assert(has_pixels_in_rect((pxl_rect_t){start_x, start_y, bounds.w, bounds.h}));
}

static void
test_pxl_draw_text_transformed_flip_hv_combined(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A";
    int x = 20, y = 20;
    pxl_writer_set_cursor(&g_w, x, y);

    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_H | PXL_FLIP_V);

    /* Should draw something without crashing */
    assert(has_pixels_in_rect((pxl_rect_t){x - 10, y - 10, 20, 20}));
}

static void
test_pxl_draw_text_transformed_flip_h_tab_advance_left(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "\tA";
    int start_x = 25, start_y = 5;
    pxl_writer_set_cursor(&g_w, start_x, start_y);

    pxl_draw_text_transformed(&g_cnv, &g_w, text, 1, PXL_FLIP_H);

    /* With flip_h, text starts at start_x + bounds.w, then moves left.
     * After \t and A, cursor should be back at start_x */
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, 1);
    assert(g_w.x == start_x);
    assert(g_w.y == start_y);
    /* Pixels should be drawn from start_x to start_x + bounds.w */
    assert(has_pixels_in_rect((pxl_rect_t){start_x, start_y, bounds.w, bounds.h}));
}

/* Regression tests for w/h swap bug */

static void
test_pxl_text_bounds_transformed_exact_single_char(void) {
    /* Regression test: verify exact dimensions for single char.
     * Would fail if w and h were swapped.
     * Note: tracking is added after each char, so 'A' = 5px + 1px tracking = 6px. */
    setup_fixture();

    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, "A", 1);
    assert(bounds.w == 6);  /* 5px glyph + 1px tracking */
    assert(bounds.h == 5);  /* glyph height */
}

static void
test_pxl_text_bounds_transformed_exact_multichar(void) {
    /* Regression test: verify width calculation for multiple chars.
     * 'ABC' = 3*(5px + 1px tracking) = 18px width, 5px height. */
    setup_fixture();

    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, "ABC", 1);
    assert(bounds.w == 18);  /* 3 chars * (5px + 1px tracking) */
    assert(bounds.h == 5);   /* glyph height */
}

static void
test_pxl_text_bounds_transformed_multiline_max_width(void) {
    /* Regression test: verify width returns max line width, not total/sum.
     * Line 1: 'A' = 6px, Line 2: 'ABC' = 18px.
     * Width must be 18 (max), not 24 (sum).
     * Height: glyph_height(5) + 1 newline * leading(6) = 11 -- only the
     * last line contributes a full glyph_height, each \n before it only
     * adds leading (leading is the full line-to-line advance already). */
    setup_fixture();

    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, "A\nABC", 1);
    assert(bounds.w == 18);  /* Max line width */
    assert(bounds.h == 11);  /* glyph_height + 1*leading */
}

static void
test_pxl_text_bounds_transformed_w_h_independence(void) {
    /* Regression test: verify w and h are calculated independently.
     * Tests that width and height don't interfere with each other. */
    setup_fixture();

    /* Wide text (width > height) */
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, "ABCDE", 1);
    /* 5 chars * (5px + 1px tracking) = 30px width */
    assert(bounds.w == 30);
    assert(bounds.h == 5);

    /* With scale, both dimensions scale independently */
    bounds = pxl_text_bounds_transformed(&g_w, "A", 3);
    /* 'A' = (5px + 1px) * 3 = 18px width, 5px * 3 = 15px height */
    assert(bounds.w == 18);
    assert(bounds.h == 15);
}

static void
test_pxl_draw_text_transformed_bounds_consistency(void) {
    /* Regression test: pxl_draw_text_transformed and
     * pxl_text_bounds_transformed must agree on multi-line height, at
     * scale > 1. leading is the full line-to-line advance (draw only
     * adds leading per \n); bounds must count glyph_height once, for the
     * last line, not once per line.
     * "A\nB\r\nC": 3 lines, 2 newlines.
     */
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "A\nB\r\nC";
    int scale = 2;
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, scale);

    int start_y = 5;
    pxl_writer_set_cursor(&g_w, 0, start_y);
    pxl_draw_text_transformed(&g_cnv, &g_w, text, scale, PXL_FLIP_NONE);

    int glyph_height = g_w.fonts[0]->glyph_height;
    int leading = g_test_font.leading;
    int newline_count = 2;

    int drawn_height = g_w.y - start_y;
    assert(drawn_height == newline_count * leading * scale);

    /* Cross-check: the bbox must exactly cover the drawn extent, same
     * invariant as test_pxl_text_draw_consistency but at scale > 1. */
    assert(bounds.h == drawn_height + glyph_height * scale);
}

/* Example tests */

static void
test_example_pxl_text_bounds_transformed(void) {
    /* Example: Get bounds of scaled text */
    setup_fixture();
    const char *text = "Hello";
    pxl_rect_t bounds = pxl_text_bounds_transformed(&g_w, text, 2);
    assert(bounds.w > 0);
    assert(bounds.h > 0);
}

static void
test_example_pxl_draw_text_transformed(void) {
    /* Example: Draw scaled text */
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    const char *text = "PXL";
    pxl_writer_set_cursor(&g_w, 5, 5);
    pxl_draw_text_transformed(&g_cnv, &g_w, text, 2, PXL_FLIP_NONE);

    assert(has_pixels_in_rect((pxl_rect_t){5, 5, 20, 20}));
}

/* Tests for pxl_next_textline */

static void
test_pxl_next_textline_empty(void) {
    const char *txt = "";
    const char *next = pxl_next_textline(txt);
    assert(next == txt); /* Returns txt when empty */
    assert(*next == '\0');
}

static void
test_pxl_next_textline_no_break(void) {
    const char *txt = "ABC";
    const char *next = pxl_next_textline(txt);
    /* Never returns NULL: returns pointer to '\0' at end of string */
    assert(next == txt + 3);
    assert(*next == '\0');
}

static void
test_pxl_next_textline_with_newline(void) {
    const char *text = "AB\nCD";
    const char *next = pxl_next_textline(text);
    assert(next == text + 3); /* Points to 'C' after '\n' */
    assert(*next == 'C');
}

static void
test_pxl_next_textline_with_carriage_return(void) {
    const char *text = "AB\rCD";
    const char *next = pxl_next_textline(text);
    assert(next == text + 3); /* Points to 'C' after '\r' */
    assert(*next == 'C');
}

static void
test_pxl_next_textline_with_crlf(void) {
    const char *text = "AB\r\nCD";
    const char *next = pxl_next_textline(text);
    assert(next == text + 4); /* Points to 'C' after '\r\n' */
    assert(*next == 'C');
}

static void
test_pxl_next_textline_at_end(void) {
    const char *text = "ABC\n";
    const char *next = pxl_next_textline(text);
    assert(next == text + 4); /* Points to '\0' after '\n' */
    assert(*next == '\0');
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
    test_pxl_draw_text_with_carriage_return();
    test_pxl_text_draw_consistency();
    test_pxl_tab_consistency_across_functions();
    test_pxl_draw_text_with_scissor();
    test_pxl_draw_text_with_offset();
    test_pxl_draw_text_proportional();

    test_pxl_text_bounds_basic();
    test_pxl_text_bounds_empty();
    test_pxl_text_bounds_height();
    test_pxl_text_bounds_with_tab();
    test_pxl_text_bounds_carriage_return_only();
    test_pxl_text_bounds_crlf();
    test_pxl_text_bounds_double_newline();
    test_pxl_text_bounds_zero_tracking();
    test_pxl_text_bounds_zero_leading();

    /* Tests for font cascade */
    test_pxl_draw_rune_cascade_basic();
    test_pxl_draw_rune_cascade_missing_rune();
    test_pxl_draw_rune_cascade_lowercase();
    test_pxl_text_bounds_cascade();

    /* Tests for truncated text helpers */
    test_pxl_text_bounds_n_basic();
    test_pxl_text_bounds_n_empty();
    test_pxl_text_bounds_n_partial();
    test_pxl_text_bounds_n_with_newline();
    test_pxl_draw_text_n_basic();
    test_pxl_draw_text_n_empty();
    test_pxl_draw_text_n_partial();

    /* Additional edge case tests for truncated text helpers */
    test_pxl_text_bounds_n_with_tab();
    test_pxl_text_bounds_n_with_carriage_return();
    test_pxl_text_bounds_n_multiline();
    test_pxl_draw_text_n_with_newline();

    /* Tests for line-based helpers */
    test_pxl_textline_bounds_basic();
    test_pxl_textline_bounds_with_newline();
    test_pxl_textline_bounds_empty();
    test_pxl_textline_bounds_with_tab();
    test_pxl_draw_textline_basic();

    /* Tests for pxl_next_textline */
    test_pxl_next_textline_example();
    test_pxl_next_textline_empty();
    test_pxl_next_textline_no_break();
    test_pxl_next_textline_with_newline();
    test_pxl_next_textline_with_carriage_return();
    test_pxl_next_textline_with_crlf();
    test_pxl_next_textline_at_end();

    /* Tests for transformed text functions */
    test_pxl_text_bounds_transformed_basic();
    test_pxl_text_bounds_transformed_scale2();
    test_pxl_draw_text_transformed_basic();
    test_pxl_draw_text_transformed_scale2();
    test_pxl_draw_text_transformed_with_flip();

    /* Tests for flip cursor behavior */
    test_pxl_draw_rune_transformed_flip_h_cursor_moves_left();
    test_pxl_draw_rune_transformed_flip_v_cursor_moves_up();
    test_pxl_draw_rune_transformed_flip_h_newline_resets_to_left();
    test_pxl_draw_rune_transformed_flip_h_newline_preserves_x();
    test_pxl_draw_text_transformed_flip_h_reversed_order();
    test_pxl_draw_text_transformed_flip_v_reversed_lines();
    test_pxl_draw_text_transformed_flip_hv_combined();
    test_pxl_draw_text_transformed_flip_h_tab_advance_left();

    /* Regression tests for transformed text functions */
    test_pxl_text_bounds_transformed_exact_single_char();
    test_pxl_text_bounds_transformed_exact_multichar();
    test_pxl_text_bounds_transformed_multiline_max_width();
    test_pxl_text_bounds_transformed_w_h_independence();
    test_pxl_draw_text_transformed_bounds_consistency();

    /* Example tests */
    test_example_pxl_text_bounds_transformed();
    test_example_pxl_draw_text_transformed();

    return 0;
}
