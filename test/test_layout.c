#include <assert.h>
#include "geom.h"
#include "layout.h"

/* --- pxl_align_rect -------------------------------------------------- */

static void
test_pxl_align_rect_left_top(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_align_rect(rect, container, PXL_ALIGN_LEFT | PXL_ALIGN_TOP);
	assert(aligned.x == 100);
	assert(aligned.y == 50);
	assert(aligned.w == 200);
	assert(aligned.h == 100);
}

static void
test_pxl_align_rect_center_center(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_align_rect(rect, container, PXL_ALIGN_H_CENTER | PXL_ALIGN_V_CENTER);
	assert(aligned.x == 100 + (800 - 200) / 2);
	assert(aligned.x == 400);
	assert(aligned.y == 50 + (600 - 100) / 2);
	assert(aligned.y == 300);
	assert(aligned.w == 200);
	assert(aligned.h == 100);
}

static void
test_pxl_align_rect_right_bottom(void) {
	pxl_rect_t rect = {0, 0, 200, 100};
	pxl_rect_t container = {100, 50, 800, 600};
	pxl_rect_t aligned = pxl_align_rect(rect, container, PXL_ALIGN_RIGHT | PXL_ALIGN_BOTTOM);
	assert(aligned.x == 100 + 800 - 200);
	assert(aligned.x == 700);
	assert(aligned.y == 50 + 600 - 100);
	assert(aligned.y == 550);
	assert(aligned.w == 200);
	assert(aligned.h == 100);
}

static void
test_pxl_align_rect_all_combinations(void) {
	pxl_rect_t rect = {0, 0, 100, 50};
	pxl_rect_t container = {0, 0, 400, 200};

	pxl_rect_t aligned;

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_LEFT | PXL_ALIGN_TOP);
	assert(aligned.x == 0 && aligned.y == 0);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_LEFT | PXL_ALIGN_V_CENTER);
	assert(aligned.x == 0 && aligned.y == (200 - 50) / 2);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_LEFT | PXL_ALIGN_BOTTOM);
	assert(aligned.x == 0 && aligned.y == 200 - 50);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_H_CENTER | PXL_ALIGN_TOP);
	assert(aligned.x == (400 - 100) / 2 && aligned.y == 0);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_H_CENTER | PXL_ALIGN_V_CENTER);
	assert(aligned.x == (400 - 100) / 2 && aligned.y == (200 - 50) / 2);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_H_CENTER | PXL_ALIGN_BOTTOM);
	assert(aligned.x == (400 - 100) / 2 && aligned.y == 200 - 50);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_RIGHT | PXL_ALIGN_TOP);
	assert(aligned.x == 400 - 100 && aligned.y == 0);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_RIGHT | PXL_ALIGN_V_CENTER);
	assert(aligned.x == 400 - 100 && aligned.y == (200 - 50) / 2);

	aligned = pxl_align_rect(rect, container, PXL_ALIGN_RIGHT | PXL_ALIGN_BOTTOM);
	assert(aligned.x == 400 - 100 && aligned.y == 200 - 50);
}

static void
test_pxl_align_rect_preserves_dimensions(void) {
	pxl_rect_t rect = {10, 20, 100, 50};
	pxl_rect_t container = {0, 0, 800, 600};

	pxl_align_t haligns[] = {PXL_ALIGN_LEFT, PXL_ALIGN_H_CENTER, PXL_ALIGN_RIGHT};
	pxl_align_t valigns[] = {PXL_ALIGN_TOP, PXL_ALIGN_V_CENTER, PXL_ALIGN_BOTTOM};

	for (int h = 0; h < 3; h++) {
		for (int v = 0; v < 3; v++) {
			pxl_rect_t aligned = pxl_align_rect(rect, container, haligns[h] | valigns[v]);
			assert(aligned.w == rect.w);
			assert(aligned.h == rect.h);
		}
	}
}

/* --- pxl_split_rect -------------------------------------------------- */

static void
test_pxl_split_rect_left(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_split_rect(&r, 20, PXL_SIDE_LEFT);
	assert(out.x == 0 && out.y == 0 && out.w == 20 && out.h == 50);
	assert(r.x == 20 && r.y == 0 && r.w == 80 && r.h == 50);
}

static void
test_pxl_split_rect_right(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_split_rect(&r, 20, PXL_SIDE_RIGHT);
	assert(out.x == 80 && out.y == 0 && out.w == 20 && out.h == 50);
	assert(r.x == 0 && r.y == 0 && r.w == 80 && r.h == 50);
}

static void
test_pxl_split_rect_top(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_split_rect(&r, 10, PXL_SIDE_TOP);
	assert(out.x == 0 && out.y == 0 && out.w == 100 && out.h == 10);
	assert(r.x == 0 && r.y == 10 && r.w == 100 && r.h == 40);
}

static void
test_pxl_split_rect_bottom(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_split_rect(&r, 10, PXL_SIDE_BOTTOM);
	assert(out.x == 0 && out.y == 40 && out.w == 100 && out.h == 10);
	assert(r.x == 0 && r.y == 0 && r.w == 100 && r.h == 40);
}

static void
test_pxl_split_rect_clamped(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_split_rect(&r, 200, PXL_SIDE_LEFT);
	assert(out.x == 0 && out.y == 0 && out.w == 100 && out.h == 50);
	assert(r.x == 100 && r.y == 0 && r.w == 0 && r.h == 50);
}

/* --- pxl_pad_rect -------------------------------------------------- */

static void
test_pxl_pad_rect_normal(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_pad_rect(r, 10, 5);
	assert(out.x == 10 && out.y == 5 && out.w == 80 && out.h == 40);
}

static void
test_pxl_pad_rect_negative(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_pad_rect(r, -10, -5);
	/* negative pad grows the rect: x -= pad_w, y -= pad_h, w += 2*pad_w, h += 2*pad_h */
	assert(out.x == -10 && out.y == -5 && out.w == 120 && out.h == 60);
}

static void
test_pxl_pad_rect_too_large(void) {
	pxl_rect_t r = {0, 0, 100, 50};
	pxl_rect_t out = pxl_pad_rect(r, 100, 100);
	assert(out.x == 50 && out.y == 25 && out.w == 0 && out.h == 0);
}

/* --- Main ----------------------------------------------------------------- */
int
main(void) {
	/* pxl_align_rect tests */
	test_pxl_align_rect_left_top();
	test_pxl_align_rect_center_center();
	test_pxl_align_rect_right_bottom();
	test_pxl_align_rect_all_combinations();
	test_pxl_align_rect_preserves_dimensions();

	/* pxl_split_rect tests */
	test_pxl_split_rect_left();
	test_pxl_split_rect_right();
	test_pxl_split_rect_top();
	test_pxl_split_rect_bottom();
	test_pxl_split_rect_clamped();

	/* pxl_pad_rect tests */
	test_pxl_pad_rect_normal();
	test_pxl_pad_rect_negative();
	test_pxl_pad_rect_too_large();

	return 0;
}
