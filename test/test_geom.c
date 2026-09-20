#include <stdbool.h>
#include "geom.h"
#include "test.h"

/* --- pxl_min/pxl_max ------------------------------------------------------ */
static void
test_min_max(void) {
	ASSERT(pxl_min(10, 20) == 10);
	ASSERT(pxl_min(20, 10) == 10);
	ASSERT(pxl_min(-5, 0) == -5);

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

/* --- pxl_rect_align -------------------------------------------------- */

static void
test_pxl_rect_align_left_top(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_rect_align(rect, container, PXL_H_LEFT | PXL_V_TOP);
	ASSERT(aligned.x == 100);
	ASSERT(aligned.y == 50);
	ASSERT(aligned.w == 200);
	ASSERT(aligned.h == 100);
}

static void
test_pxl_rect_align_center_center(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_rect_align(rect, container, PXL_H_CENTER | PXL_V_CENTER);
	ASSERT(aligned.x == 100 + (800 - 200) / 2);
	ASSERT(aligned.x == 400);
	ASSERT(aligned.y == 50 + (600 - 100) / 2);
	ASSERT(aligned.y == 300);
	ASSERT(aligned.w == 200);
	ASSERT(aligned.h == 100);
}

static void
test_pxl_rect_align_right_bottom(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_rect_align(rect, container, PXL_H_RIGHT | PXL_V_BOTTOM);
	ASSERT(aligned.x == 100 + 800 - 200);
	ASSERT(aligned.x == 700);
	ASSERT(aligned.y == 50 + 600 - 100);
	ASSERT(aligned.y == 550);
	ASSERT(aligned.w == 200);
	ASSERT(aligned.h == 100);
}

static void
test_pxl_rect_align_all_combinations(void) {
	pxl_rect_t rect = {0, 0, 100, 50};
	pxl_rect_t container = {0, 0, 400, 200};

	pxl_rect_t aligned;

	aligned = pxl_rect_align(rect, container, PXL_H_LEFT | PXL_V_TOP);
	ASSERT(aligned.x == 0 && aligned.y == 0);

	aligned = pxl_rect_align(rect, container, PXL_H_LEFT | PXL_V_CENTER);
	ASSERT(aligned.x == 0 && aligned.y == (200 - 50) / 2);

	aligned = pxl_rect_align(rect, container, PXL_H_LEFT | PXL_V_BOTTOM);
	ASSERT(aligned.x == 0 && aligned.y == 200 - 50);

	aligned = pxl_rect_align(rect, container, PXL_H_CENTER | PXL_V_TOP);
	ASSERT(aligned.x == (400 - 100) / 2 && aligned.y == 0);

	aligned = pxl_rect_align(rect, container, PXL_H_CENTER | PXL_V_CENTER);
	ASSERT(aligned.x == (400 - 100) / 2 && aligned.y == (200 - 50) / 2);

	aligned = pxl_rect_align(rect, container, PXL_H_CENTER | PXL_V_BOTTOM);
	ASSERT(aligned.x == (400 - 100) / 2 && aligned.y == 200 - 50);

	aligned = pxl_rect_align(rect, container, PXL_H_RIGHT | PXL_V_TOP);
	ASSERT(aligned.x == 400 - 100 && aligned.y == 0);

	aligned = pxl_rect_align(rect, container, PXL_H_RIGHT | PXL_V_CENTER);
	ASSERT(aligned.x == 400 - 100 && aligned.y == (200 - 50) / 2);

	aligned = pxl_rect_align(rect, container, PXL_H_RIGHT | PXL_V_BOTTOM);
	ASSERT(aligned.x == 400 - 100 && aligned.y == 200 - 50);
}

static void
test_pxl_rect_align_preserves_dimensions(void) {
	pxl_rect_t rect = {10, 20, 100, 50};
	pxl_rect_t container = {0, 0, 800, 600};

	pxl_align_t haligns[] = {PXL_H_LEFT, PXL_H_CENTER, PXL_H_RIGHT};
	pxl_align_t valigns[] = {PXL_V_TOP, PXL_V_CENTER, PXL_V_BOTTOM};

	for (int h = 0; h < 3; h++) {
		for (int v = 0; v < 3; v++) {
			pxl_rect_t aligned = pxl_rect_align(rect, container, haligns[h] | valigns[v]);
			ASSERT(aligned.w == rect.w);
			ASSERT(aligned.h == rect.h);
		}
	}
}

/* Example test from geom.h documentation */
static void
test_example_pxl_rect_align(void) {
	pxl_rect_t bounds = {0, 0, 100, 50};
	pxl_rect_t container = {0, 0, 800, 600};
	pxl_rect_t aligned = pxl_rect_align(bounds, container, PXL_H_CENTER | PXL_V_CENTER);
	ASSERT(aligned.w == bounds.w);
	ASSERT(aligned.h == bounds.h);
	ASSERT(aligned.x == 350);  /* (800 - 100) / 2 = 350 */
	ASSERT(aligned.y == 275);  /* (600 - 50) / 2 = 275 */
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

	/* pxl_rect_align tests */
	test_pxl_rect_align_left_top();
	test_pxl_rect_align_center_center();
	test_pxl_rect_align_right_bottom();
	test_pxl_rect_align_all_combinations();
	test_pxl_rect_align_preserves_dimensions();

	/* Example tests */
	test_example_pxl_rect_align();

	return 0;
}
