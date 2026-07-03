#include <stdbool.h>
#include "geom.h"
#include "stest/stest.h"


/* min&max tests ----------------------------------------------------------- */

static void
test_min_basic(void) {
	ST_CHECK(pxl_min(10, 20) == 10, "pxl_min(10,20) should be 10");
	ST_CHECK(pxl_min(20, 10) == 10, "pxl_min(20,10) should be 10");
	ST_CHECK(pxl_min(-5, 0) == -5, "pxl_min(-5,0) should be -5");
}

static void
test_max_basic(void) {
	ST_CHECK(pxl_max(10, 20) == 20, "pxl_max(10,20) should be 20");
	ST_CHECK(pxl_max(20, 10) == 20, "pxl_max(20,10) should be 20");
	ST_CHECK(pxl_max(-5, 0) == 0, "pxl_max(-5,0) should be 0");
}


/* pxl_clip_span tests -------------------------------------------------------- */

static void
test_pxl_clip_span_fully_left(void) {
	int x = -10, w = 5;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(!inside, "span fully left should be clipped");
}

static void
test_pxl_clip_span_fully_right(void) {
	int x = 110, w = 5;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(!inside, "span fully right should be clipped");
}

static void
test_pxl_clip_span_fully_inside(void) {
	int x = 10, w = 20;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(inside, "span fully inside should not be clipped");
	ST_CHECK(out.x == 10, "x should be unchanged, got %d", x);
	ST_CHECK(out.w == 20, "w should be unchanged, got %d", w);
}

static void
test_pxl_clip_span_partial_left(void) {
	int x = -5, w = 20;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(inside, "span partially inside on left should be clipped");
	ST_CHECK(out.x == 0, "x should be clamped to 0, got %d", x);
	ST_CHECK(out.w == 15, "w should be 15, got %d", w);
}

static void
test_pxl_clip_span_partial_right(void) {
	int x = 90, w = 20;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(inside, "span partially inside on right should be clipped");
	ST_CHECK(out.x == 90, "x should be unchanged, got %d", x);
	ST_CHECK(out.w == 10, "w should be 10, got %d", w);
}

static void
test_pxl_clip_span_zero_width(void) {
	int x = 10, w = 0;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(!inside, "zero-width span should be clipped");
}

static void
test_pxl_clip_span_spanning_bounds(void) {
	int x = -10, w = 120;
	pxl_span_t out;
	bool inside = pxl_clip_span((pxl_span_t){x, w}, (pxl_span_t){0, 100}, &out);
	ST_CHECK(inside, "span spanning bounds should be clipped");
	ST_CHECK(out.x == 0, "x should be clamped to 0, got %d", x);
	ST_CHECK(out.w == 100, "w should be 100, got %d", w);
}


/* pxl_clip_rect tests ------------------------------------------- */

static void
test_pxl_clip_rect_fully_inside(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	pxl_rect_t out;
	bool inside = pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out);
	ST_CHECK(inside, "fully inside rect should return true");
	ST_CHECK(out.x == 10 && out.y == 10 && out.w == 20 && out.h == 20,
	         "rect should be unchanged");
}

static void
test_pxl_clip_rect_fully_outside(void) {
	pxl_rect_t r = {110, 110, 20, 20};
	pxl_rect_t out;
	bool inside = pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out);
	ST_CHECK(!inside, "fully outside rect should return false");
}

static void
test_pxl_clip_rect_partial(void) {
	pxl_rect_t r = {-10, -10, 40, 40};
	pxl_rect_t out;
	bool inside = pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out);
	ST_CHECK(inside, "partially inside rect should return true");
	ST_CHECK(out.x == 0 && out.y == 0 && out.w == 30 && out.h == 30,
	         "clipped rect should be {0,0,30,30}");
}


/* pxl_in_rect tests -------------------------------------------------------- */

static void
test_pxl_in_rect_inside(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(pxl_in_rect(15, 15, r), "point (15,15) should be inside {10,10,20,20}");
	ST_CHECK(pxl_in_rect(10, 10, r), "point (10,10) should be inside (top-left corner inclusive)");
	ST_CHECK(pxl_in_rect(29, 29, r), "point (29,29) should be inside (just before bottom-right)");
}

static void
test_pxl_in_rect_outside_left(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(!pxl_in_rect(9, 15, r), "point (9,15) should be outside (left of rect)");
}

static void
test_pxl_in_rect_outside_right(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(!pxl_in_rect(30, 15, r), "point (30,15) should be outside (right of rect, exclusive)");
}

static void
test_pxl_in_rect_outside_above(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(!pxl_in_rect(15, 9, r), "point (15,9) should be outside (above rect)");
}

static void
test_pxl_in_rect_outside_below(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(!pxl_in_rect(15, 30, r), "point (15,30) should be outside (below rect, exclusive)");
}

static void
test_pxl_in_rect_corners(void) {
	pxl_rect_t r = {10, 10, 20, 20};
	ST_CHECK(pxl_in_rect(10, 10, r), "top-left corner (10,10) should be inside (inclusive)");
	ST_CHECK(!pxl_in_rect(30, 10, r), "top-right corner (30,10) should be outside (exclusive)");
	ST_CHECK(!pxl_in_rect(10, 30, r), "bottom-left corner (10,30) should be outside (exclusive)");
	ST_CHECK(!pxl_in_rect(30, 30, r), "bottom-right corner (30,30) should be outside (exclusive)");
}

static void
test_pxl_in_rect_zero_size(void) {
	pxl_rect_t r = {10, 10, 0, 0};
	ST_CHECK(!pxl_in_rect(10, 10, r), "point (10,10) should be outside zero-size rect");
	ST_CHECK(!pxl_in_rect(15, 15, r), "point (15,15) should be outside zero-size rect");
}


/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[])
{
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		ST_T(test_min_basic),
		ST_T(test_max_basic),
		ST_T(test_pxl_clip_span_fully_left),
		ST_T(test_pxl_clip_span_fully_right),
		ST_T(test_pxl_clip_span_fully_inside),
		ST_T(test_pxl_clip_span_partial_left),
		ST_T(test_pxl_clip_span_partial_right),
		ST_T(test_pxl_clip_span_zero_width),
		ST_T(test_pxl_clip_span_spanning_bounds),
		ST_T(test_pxl_clip_rect_fully_inside),
		ST_T(test_pxl_clip_rect_fully_outside),
		ST_T(test_pxl_clip_rect_partial),
		ST_T(test_pxl_in_rect_inside),
		ST_T(test_pxl_in_rect_outside_left),
		ST_T(test_pxl_in_rect_outside_right),
		ST_T(test_pxl_in_rect_outside_above),
		ST_T(test_pxl_in_rect_outside_below),
		ST_T(test_pxl_in_rect_corners),
		ST_T(test_pxl_in_rect_zero_size)
	);
}
