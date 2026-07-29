#include <stdbool.h>
#include "geom.h"
#include "test.h"

/* --- pxl_min/pxl_max ------------------------------------------------------ */
static void
test_min_max(void) {
	// pxl_min
	ASSERT(pxl_min(10, 20) == 10);
	ASSERT(pxl_min(20, 10) == 10);
	ASSERT(pxl_min(-5, 0) == -5);

	// pxl_max
	ASSERT(pxl_max(10, 20) == 20);
	ASSERT(pxl_max(20, 10) == 20);
	ASSERT(pxl_max(-5, 0) == 0);
}

/* --- pxl_clip_span -------------------------------------------------------- */
static void
test_clip_span_fully_outside(void) {
	pxl_span_t out;
	ASSERT(!pxl_clip_span((pxl_span_t){-10, 5}, (pxl_span_t){0, 100}, &out));
	ASSERT(!pxl_clip_span((pxl_span_t){110, 5}, (pxl_span_t){0, 100}, &out));
}

static void
test_clip_span_fully_inside(void) {
	pxl_span_t out;
	ASSERT(pxl_clip_span((pxl_span_t){10, 20}, (pxl_span_t){0, 100}, &out));
	ASSERT(out.x == 10 && out.w == 20);
}

static void
test_clip_span_partial(void) {
	pxl_span_t out;
	ASSERT(pxl_clip_span((pxl_span_t){-5, 20}, (pxl_span_t){0, 100}, &out));
	ASSERT(out.x == 0 && out.w == 15);

	ASSERT(pxl_clip_span((pxl_span_t){90, 20}, (pxl_span_t){0, 100}, &out));
	ASSERT(out.x == 90 && out.w == 10);
}

static void
test_clip_span_edge_cases(void) {
	pxl_span_t out;
	ASSERT(!pxl_clip_span((pxl_span_t){10, 0}, (pxl_span_t){0, 100}, &out));

	ASSERT(pxl_clip_span((pxl_span_t){-10, 120}, (pxl_span_t){0, 100}, &out));
	ASSERT(out.x == 0 && out.w == 100);
}

/* --- pxl_clip_rect -------------------------------------------------------- */
static void
test_clip_rect_fully_inside(void) {
	pxl_rect_t out;
	pxl_rect_t r = {10, 10, 20, 20};
	ASSERT(pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out));
	ASSERT(out.x == 10 && out.y == 10 && out.w == 20 && out.h == 20);
}

static void
test_clip_rect_fully_outside(void) {
	pxl_rect_t out;
	pxl_rect_t r = {110, 110, 20, 20};
	ASSERT(!pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out));
}

static void
test_clip_rect_partial(void) {
	pxl_rect_t out;
	pxl_rect_t r = {-10, -10, 40, 40};
	ASSERT(pxl_clip_rect(r, (pxl_rect_t){0, 0, 100, 100}, &out));
	ASSERT(out.x == 0 && out.y == 0 && out.w == 30 && out.h == 30);
}

/* --- Main ----------------------------------------------------------------- */
int
main(void) {
	test_min_max();

	test_clip_span_fully_outside();
	test_clip_span_fully_inside();
	test_clip_span_partial();
	test_clip_span_edge_cases();

	test_clip_rect_fully_inside();
	test_clip_rect_fully_outside();
	test_clip_rect_partial();

	return 0;
}
