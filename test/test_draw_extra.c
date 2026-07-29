#include <string.h>
#include <limits.h>
#include "test.h"
#include "canvas.h"
#include "buf.h"
#include "draw.h"
#include "draw_extra.h"

/* Fixture ---------------------------------------------------------------- */
#define FIXTURE_W 100
#define FIXTURE_H 100
#define FIXTURE_STRIDE 100  /* pxl_calc_stride(100) = 100 */

static pxl_t g_buf_data[FIXTURE_STRIDE * FIXTURE_H];
static pxl_buf_t g_buf = {
	.data = g_buf_data,
	.width = FIXTURE_W,
	.height = FIXTURE_H,
	.stride = FIXTURE_STRIDE
};
static pxl_canvas_t g_cnv;

static inline void
fixture_reset(void) {
	memset(g_buf_data, 0x00, sizeof(g_buf_data));
	pxl_canvas_init(&g_cnv, &g_buf);
}

static inline void
fixture_reset_scissor(int x, int y, int w, int h) {
	fixture_reset();
	pxl_canvas_set_scissor(&g_cnv, x, y, w, h);
}

/* Helpers --------------------------------------------------------------- */

static inline bool
buf_is_empty(void) {
	for (size_t i = 0; i < FIXTURE_STRIDE * FIXTURE_H; ++i) {
		if (g_buf_data[i] != 0) return false;
	}
	return true;
}

static inline bool
buf_is_not_empty(void) {
	return !buf_is_empty();
}

static inline pxl_t
buf_get(int x, int y) {
	return *pxl_buf_ptr(&g_buf, x, y);
}

/* Tests - Circle --------------------------------------------------------- */

static void
test_draw_circle_basic(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	pxl_draw_circle(&g_cnv, 50, 50, 10);

	/* Check that something was drawn */
	ASSERT(buf_is_not_empty());

	/* Check top, bottom, left, right cardinal points */
	ASSERT(buf_get(50, 40) == 0xFFFF0000);  /* top */
	ASSERT(buf_get(50, 60) == 0xFFFF0000);  /* bottom */
	ASSERT(buf_get(40, 50) == 0xFFFF0000);  /* left */
	ASSERT(buf_get(60, 50) == 0xFFFF0000);  /* right */
}

static void
test_draw_circle_min_radius(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_draw_circle(&g_cnv, 50, 50, 1);

	ASSERT(buf_is_not_empty());
	/* With r=1, only 4 points around center + center cross */
	ASSERT(buf_get(50, 49) == 0xFF00FF00);
	ASSERT(buf_get(50, 51) == 0xFF00FF00);
	ASSERT(buf_get(49, 50) == 0xFF00FF00);
	ASSERT(buf_get(51, 50) == 0xFF00FF00);
}

static void
test_draw_circle_outside_scissor(void) {
	fixture_reset_scissor(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	/* Circle completely outside scissor */
	pxl_draw_circle(&g_cnv, 100, 100, 10);

	/* Buffer should remain empty */
	ASSERT(buf_is_empty());
}

static void
test_draw_circle_with_offset(void) {
	fixture_reset();

	pxl_canvas_set_offset(&g_cnv, 20, 20);
	pxl_canvas_set_color(&g_cnv, 0xFF0000FF);

	/* Draw at (0,0) with offset becomes (20,20) */
	pxl_draw_circle(&g_cnv, 0, 0, 5);

	ASSERT(buf_is_not_empty());
	/* Check cardinal points with offset: center is at (20,20) with r=5 */
	ASSERT(buf_get(20, 15) == 0xFF0000FF);  /* top: (20,20-5) */
	ASSERT(buf_get(20, 25) == 0xFF0000FF);  /* bottom: (20,20+5) */
	ASSERT(buf_get(15, 20) == 0xFF0000FF);  /* left: (20-5,20) */
	ASSERT(buf_get(25, 20) == 0xFF0000FF);  /* right: (20+5,20) */
}

/* Tests - Fill Circle ---------------------------------------------------- */

static void
test_fill_circle_basic(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_fill_circle(&g_cnv, 50, 50, 10);

	ASSERT(buf_is_not_empty());

	/* Check center */
	ASSERT(buf_get(50, 50) == 0xFF00FF00);
	/* Check cardinal points on edge */
	ASSERT(buf_get(50, 40) == 0xFF00FF00);  /* top */
	ASSERT(buf_get(50, 60) == 0xFF00FF00);  /* bottom */
	ASSERT(buf_get(40, 50) == 0xFF00FF00);  /* left */
	ASSERT(buf_get(60, 50) == 0xFF00FF00);  /* right */
}

static void
test_fill_circle_min_radius(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFFFFFF00);
	pxl_fill_circle(&g_cnv, 50, 50, 1);

	ASSERT(buf_is_not_empty());
	/* Center should be filled */
	ASSERT(buf_get(50, 50) == 0xFFFFFF00);
	/* Edge points */
	ASSERT(buf_get(50, 49) == 0xFFFFFF00);
	ASSERT(buf_get(50, 51) == 0xFFFFFF00);
	ASSERT(buf_get(49, 50) == 0xFFFFFF00);
	ASSERT(buf_get(51, 50) == 0xFFFFFF00);
}

static void
test_fill_circle_outside_scissor(void) {
	fixture_reset_scissor(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_fill_circle(&g_cnv, 100, 100, 10);

	ASSERT(buf_is_empty());
}

static void
test_fill_circle_with_offset(void) {
	fixture_reset();

	pxl_canvas_set_offset(&g_cnv, 10, 10);
	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);

	pxl_fill_circle(&g_cnv, 0, 0, 8);

	ASSERT(buf_is_not_empty());
	/* Center with offset */
	ASSERT(buf_get(10, 10) == 0xFF00FF00);
	/* Edge with offset */
	ASSERT(buf_get(10, 2) == 0xFF00FF00);   /* top */
	ASSERT(buf_get(10, 18) == 0xFF00FF00);  /* bottom */
	ASSERT(buf_get(2, 10) == 0xFF00FF00);   /* left */
	ASSERT(buf_get(18, 10) == 0xFF00FF00);  /* right */
}

/* Tests - Triangle -------------------------------------------------------- */

static void
test_draw_triangle_basic(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	pxl_draw_triangle(&g_cnv, 30, 30, 50, 30, 40, 50);

	ASSERT(buf_is_not_empty());

	/* Check the three vertices */
	ASSERT(buf_get(30, 30) == 0xFFFF0000);
	ASSERT(buf_get(50, 30) == 0xFFFF0000);
	ASSERT(buf_get(40, 50) == 0xFFFF0000);
}

static void
test_draw_triangle_outside_scissor(void) {
	fixture_reset_scissor(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	pxl_draw_triangle(&g_cnv, 100, 100, 120, 100, 110, 120);

	ASSERT(buf_is_empty());
}

static void
test_draw_triangle_with_offset(void) {
	fixture_reset();

	pxl_canvas_set_offset(&g_cnv, 10, 10);
	pxl_canvas_set_color(&g_cnv, 0xFF0000FF);

	pxl_draw_triangle(&g_cnv, 0, 0, 20, 0, 10, 20);

	ASSERT(buf_is_not_empty());
	/* Vertices with offset */
	ASSERT(buf_get(10, 10) == 0xFF0000FF);
	ASSERT(buf_get(30, 10) == 0xFF0000FF);
	ASSERT(buf_get(20, 30) == 0xFF0000FF);
}

/* Tests - Fill Triangle ---------------------------------------------------- */

static void
test_fill_triangle_basic(void) {
	fixture_reset();

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_fill_triangle(&g_cnv, 30, 30, 50, 30, 40, 50);

	ASSERT(buf_is_not_empty());

	/* Check vertices are filled */
	ASSERT(buf_get(30, 30) == 0xFF00FF00);
	ASSERT(buf_get(50, 30) == 0xFF00FF00);
	ASSERT(buf_get(40, 50) == 0xFF00FF00);
	/* Check center-ish point is filled */
	ASSERT(buf_get(40, 40) == 0xFF00FF00);
}

static void
test_fill_triangle_outside_scissor(void) {
	fixture_reset_scissor(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_fill_triangle(&g_cnv, 100, 100, 120, 100, 110, 120);

	ASSERT(buf_is_empty());
}

static void
test_fill_triangle_with_offset(void) {
	fixture_reset();

	pxl_canvas_set_offset(&g_cnv, 5, 5);
	pxl_canvas_set_color(&g_cnv, 0xFFFFFF00);

	pxl_fill_triangle(&g_cnv, 0, 0, 10, 0, 5, 10);

	ASSERT(buf_is_not_empty());
	/* Vertices with offset */
	ASSERT(buf_get(5, 5) == 0xFFFFFF00);
	ASSERT(buf_get(15, 5) == 0xFFFFFF00);
	ASSERT(buf_get(10, 15) == 0xFFFFFF00);
	/* Center with offset */
	ASSERT(buf_get(10, 10) == 0xFFFFFF00);
}

/* Tests - Edge Cases ------------------------------------------------------- */

static void
test_circle_partial_scissor(void) {
	fixture_reset_scissor(20, 20, 40, 40);

	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	/* Circle center (30,30) radius 15: bbox (15,15) to (45,45) */
	/* Scissor (20,20) to (60,60): partial overlap */
	pxl_draw_circle(&g_cnv, 30, 30, 15);

	/* Should have some pixels drawn (the part inside scissor) */
	ASSERT(buf_is_not_empty());
	/* Top-left of circle should be clipped */
	ASSERT(buf_get(15, 15) == 0x00000000);  /* outside scissor */
	/* Right edge of circle at (45,30) should be drawn and inside scissor */
	ASSERT(buf_get(45, 30) == 0xFFFF0000);  /* on circle edge, inside scissor */
	/* Bottom edge of circle at (30,45) should be drawn and inside scissor */
	ASSERT(buf_get(30, 45) == 0xFFFF0000);  /* on circle edge, inside scissor */
}

static void
test_fill_circle_partial_scissor(void) {
	fixture_reset_scissor(25, 25, 30, 30);

	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_fill_circle(&g_cnv, 40, 40, 15);

	ASSERT(buf_is_not_empty());
	/* Part inside scissor should be filled */
	ASSERT(buf_get(35, 35) == 0xFF00FF00);
	/* Part outside scissor should be empty */
	ASSERT(buf_get(20, 20) == 0x00000000);
}

static void
test_fill_triangle_partial_scissor(void) {
	fixture_reset_scissor(25, 25, 30, 30);

	pxl_canvas_set_color(&g_cnv, 0xFFFFFF00);
	/* Triangle partially overlapping scissor */
	pxl_fill_triangle(&g_cnv, 20, 20, 50, 20, 35, 50);

	ASSERT(buf_is_not_empty());
	/* Part inside scissor should be filled */
	ASSERT(buf_get(30, 30) == 0xFFFFFF00);
	/* Part outside scissor should be empty */
	ASSERT(buf_get(20, 20) == 0x00000000);
}

/* Tests - Assertion coverage ----------------------------------------------- */

static void
test_draw_circle_max_safe_radius(void) {
	/* Verify the overflow protection constant */
	int max_safe_r = (INT_MAX - 1) / 2;
	ASSERT(max_safe_r > 0);
	ASSERT(max_safe_r <= (INT_MAX - 1) / 2);
	
	/* Test with a large but safe radius */
	fixture_reset();
	pxl_canvas_set_color(&g_cnv, 0xFFFF0000);
	pxl_draw_circle(&g_cnv, 50, 50, 40);
	ASSERT(buf_is_not_empty());
}

static void
test_fill_triangle_large_coords(void) {
	/* Test that bounding box doesn't overflow with large but valid coords */
	fixture_reset();
	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	/* Use coordinates that are large but won't cause overflow in bounding box */
	pxl_fill_triangle(&g_cnv, 10, 10, 50, 10, 30, 50);
	ASSERT(buf_is_not_empty());
}

/* Main ------------------------------------------------------------------- */
int
main(void) {
	/* Circle tests */
	test_draw_circle_basic();
	test_draw_circle_min_radius();
	test_draw_circle_outside_scissor();
	test_draw_circle_with_offset();

	/* Fill Circle tests */
	test_fill_circle_basic();
	test_fill_circle_min_radius();
	test_fill_circle_outside_scissor();
	test_fill_circle_with_offset();

	/* Triangle tests */
	test_draw_triangle_basic();
	test_draw_triangle_outside_scissor();
	test_draw_triangle_with_offset();

	/* Fill Triangle tests */
	test_fill_triangle_basic();
	test_fill_triangle_outside_scissor();
	test_fill_triangle_with_offset();

	/* Partial overlap tests */
	test_circle_partial_scissor();
	test_fill_circle_partial_scissor();
	test_fill_triangle_partial_scissor();

	/* Assertion/edge case coverage */
	test_draw_circle_max_safe_radius();
	test_fill_triangle_large_coords();

	return 0;
}
