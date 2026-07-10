#include <string.h>
#include "canvas.h"
#include "ascii.h"
#include "stest/stest.h"

#define COLOR_WHITE 0xFFFFFFFFU

/* Fixture ----------------------------------------------------------------- */
typedef struct {
	pxl_buf_t pb;
	pxl_canvas_t cnv;
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

/* Tests ----------------------------------------------------------------- */
static void
test_pxl_draw_char_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 2, y = 2;
	pxl_draw_char(&f.cnv, x, y, 'A');

	/* Check bounding box */
	pxl_rect_t expected = pxl_char_bounds('A');
	expected.x += x;
	expected.y += y;

	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'A' bounding box");

	fixture_deinit(&f);
}

static void
test_pxl_draw_char_with_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_scissor(&f.cnv, 8, 8, 6, 6);
	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 5, y = 5;
	pxl_draw_char(&f.cnv, x, y, 'X');

	/* Check that part of the char is drawn in scissor area */
	ST_CHECK(has_pixels_in_rect(&f.pb, (pxl_rect_t){8, 8, 6, 6}),
	         "No pixels drawn in scissor area");

	fixture_deinit(&f);
}

static void
test_pxl_draw_char_non_printable(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 5, y = 5;
	pxl_draw_char(&f.cnv, x, y, '\a');  /* Non-printable */

	/* Should be replaced by space (no pixels drawn) */
	pxl_rect_t expected = pxl_char_bounds(' ');
	expected.x += x;
	expected.y += y;

	ST_CHECK(!has_pixels_in_rect(&f.pb, expected),
	         "Non-printable char drew pixels");

	fixture_deinit(&f);
}

static void
test_pxl_draw_char_out_of_range(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 5, y = 5;
	pxl_draw_char(&f.cnv, x, y, 128);  /* > 127 */

	/* Should be replaced by '?' */
	pxl_rect_t expected = pxl_char_bounds('?');
	expected.x += x;
	expected.y += y;

	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "Char 128 not replaced by '?'");

	fixture_deinit(&f);
}

static void
test_pxl_draw_str_basic(void) {
	int w = 40, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 5, y = 5;
	pxl_draw_str(&f.cnv, x, y, "HI");

	/* Check global bounding box */
	pxl_rect_t expected = pxl_str_bounds("HI");
	expected.x += x;
	expected.y += y;

	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'HI' bounding box");

	fixture_deinit(&f);
}

static void
test_pxl_draw_str_empty(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	pxl_draw_str(&f.cnv, 5, 5, "");  /* Empty string */

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
test_pxl_draw_str_with_newline(void) {
	int w = 20, h = 30;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 5, y = 5;
	pxl_draw_str(&f.cnv, x, y, "A\nB");

	/* Check global bounding box */
	pxl_rect_t expected = pxl_str_bounds("A\nB");
	expected.x += x;
	expected.y += y;

	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "No pixels drawn in 'A\\nB' bounding box");

	fixture_deinit(&f);
}

static void
test_pxl_draw_str_with_offset(void) {
	int w = 30, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_offset(&f.cnv, 5, 3);
	pxl_t color = COLOR_WHITE;
	pxl_canvas_set_color(&f.cnv, color);

	int x = 0, y = 0;
	pxl_draw_str(&f.cnv, x, y, "PXL");

	/* Should appear at (5,3) due to offset */
	pxl_rect_t expected = pxl_str_bounds("PXL");
	expected.x += 5;
	expected.y += 3;

	ST_CHECK(has_pixels_in_rect(&f.pb, expected),
	         "Offset not applied to draw_str");

	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Char tests */
		ST_T(test_pxl_draw_char_basic),
		ST_T(test_pxl_draw_char_with_scissor),
		ST_T(test_pxl_draw_char_non_printable),
		ST_T(test_pxl_draw_char_out_of_range),
		/* String tests */
		ST_T(test_pxl_draw_str_basic),
		ST_T(test_pxl_draw_str_empty),
		ST_T(test_pxl_draw_str_with_newline),
		ST_T(test_pxl_draw_str_with_offset)
	);
}
