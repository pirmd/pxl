#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "buf.h"
#include "canvas.h"
#include "geom.h"
#include "text_basic.h"

#define COLOR_WHITE 0xFFFFFFFFU
#define FIXTURE_W 40
#define FIXTURE_H 40

/* Fixture */
static pxl_buf_t g_pb;
static pxl_canvas_t g_cnv;
static pxl_t g_buf_data[FIXTURE_H][FIXTURE_W];

static void
setup_fixture(void) {
    g_pb.width = FIXTURE_W;
    g_pb.height = FIXTURE_H;
    g_pb.stride = FIXTURE_W;
    g_pb.data = &g_buf_data[0][0];
    pxl_canvas_init(&g_cnv, &g_pb);
    memset(g_buf_data, 0, sizeof(g_buf_data));
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

/* Tests for pxl_draw_char */
static void
test_pxl_draw_char_basic(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 2, y = 2;
    pxl_draw_char(&g_cnv, x, y, 'A');

    pxl_rect_t expected = pxl_char_bounds('A');
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_char_with_scissor(void) {
    setup_fixture();
    pxl_canvas_set_scissor(&g_cnv, 8, 8, 6, 6);
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_char(&g_cnv, x, y, 'X');

    assert(has_pixels_in_rect((pxl_rect_t){8, 8, 6, 6}));
}

static void
test_pxl_draw_char_non_printable(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_char(&g_cnv, x, y, '\a');

    pxl_rect_t expected = pxl_char_bounds(' ');
    expected.x += x;
    expected.y += y;

    assert(!has_pixels_in_rect(expected));
}

static void
test_pxl_draw_char_out_of_range(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_char(&g_cnv, x, y, 128);

    pxl_rect_t expected = pxl_char_bounds('?');
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

/* Tests for pxl_draw_str */
static void
test_pxl_draw_str_basic(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_str(&g_cnv, x, y, "HI");

    pxl_rect_t expected = pxl_str_bounds("HI");
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_str_empty(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    pxl_draw_str(&g_cnv, 5, 5, "");

    for (int y = 0; y < FIXTURE_H; y++) {
        for (int x = 0; x < FIXTURE_W; x++) {
            assert(g_buf_data[y][x] == 0);
        }
    }
}

static void
test_pxl_draw_str_with_newline(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_str(&g_cnv, x, y, "A\nB");

    pxl_rect_t expected = pxl_str_bounds("A\nB");
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_str_with_offset(void) {
    setup_fixture();
    pxl_canvas_set_offset(&g_cnv, 5, 3);
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 0, y = 0;
    pxl_draw_str(&g_cnv, x, y, "PXL");

    pxl_rect_t expected = pxl_str_bounds("PXL");
    expected.x += 5;
    expected.y += 3;

    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_str_with_tab(void) {
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 5, y = 5;
    pxl_draw_str(&g_cnv, x, y, "A\tB");

    pxl_rect_t expected = pxl_str_bounds("A\tB");
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

static void
test_pxl_draw_str_with_trailing_tab(void) {
    /* Regression test: draw and pxl_str_bounds must agree even when the
     * tab is the last (and widest) element of the line. */
    setup_fixture();
    pxl_canvas_set_color(&g_cnv, COLOR_WHITE);

    int x = 2, y = 2;
    pxl_draw_str(&g_cnv, x, y, "A\t");

    pxl_rect_t expected = pxl_str_bounds("A\t");
    expected.x += x;
    expected.y += y;

    assert(has_pixels_in_rect(expected));
}

/* Tests for pxl_char_bounds */
static void
test_pxl_char_bounds_basic(void) {
    pxl_rect_t bounds = pxl_char_bounds('A');
    assert(bounds.w == 8 && bounds.h == 8);
}

static void
test_pxl_char_bounds_special(void) {
    pxl_rect_t tab_bounds = pxl_char_bounds('\t');
    assert(tab_bounds.w == 36 && tab_bounds.h == 8);

    pxl_rect_t space_bounds = pxl_char_bounds(' ');
    assert(space_bounds.w == 8 && space_bounds.h == 8);

    pxl_rect_t question_bounds = pxl_char_bounds(128);
    assert(question_bounds.w == 8 && question_bounds.h == 8);
}

/* Tests for pxl_str_bounds */
static void
test_pxl_str_bounds_basic(void) {
    pxl_rect_t bounds = pxl_str_bounds("HI");
    assert(bounds.w == 18 && bounds.h == 8);
}

static void
test_pxl_str_bounds_empty(void) {
    pxl_rect_t bounds = pxl_str_bounds("");
    assert(bounds.w == 0 && bounds.h == 0);
}

static void
test_pxl_str_bounds_with_newline(void) {
    pxl_rect_t bounds = pxl_str_bounds("A\nB");
    assert(bounds.w == 9 && bounds.h == 18);
}

static void
test_pxl_str_bounds_trailing_tab(void) {
    /* Regression test: a tab as the last char must still extend max_x.
     * 'A' reaches x=9, then \t snaps forward to the next 36px tab stop. */
    pxl_rect_t bounds = pxl_str_bounds("A\t");
    assert(bounds.w == 36);
    assert(bounds.h == 8);
}

static void
test_pxl_str_bounds_only_tab(void) {
    /* Regression test: a line made of only a tab must not report w == 0. */
    pxl_rect_t bounds = pxl_str_bounds("\t");
    assert(bounds.w == 36);
    assert(bounds.h == 8);
}

int
main(void) {
    test_pxl_draw_char_basic();
    test_pxl_draw_char_with_scissor();
    test_pxl_draw_char_non_printable();
    test_pxl_draw_char_out_of_range();

    test_pxl_draw_str_basic();
    test_pxl_draw_str_empty();
    test_pxl_draw_str_with_newline();
    test_pxl_draw_str_with_offset();
    test_pxl_draw_str_with_tab();
    test_pxl_draw_str_with_trailing_tab();

    test_pxl_char_bounds_basic();
    test_pxl_char_bounds_special();
    test_pxl_str_bounds_basic();
    test_pxl_str_bounds_empty();
    test_pxl_str_bounds_with_newline();
    test_pxl_str_bounds_trailing_tab();
    test_pxl_str_bounds_only_tab();

    return 0;
}
