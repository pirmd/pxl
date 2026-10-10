#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include "buf.h"
#include "canvas.h"
#include "geom.h"

/* Fixture ----------------------------------------------------------------- */
#define FIXTURE_W 101
#define FIXTURE_H 128
#define FIXTURE_STRIDE 104  /* pxl_calc_stride(101) = 104 */

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

/* Helpers ----------------------------------------------------------------- */

static bool
in_scissor(int x, int y) {
	const pxl_rect_t *s = &g_cnv.scissor;
	return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

static void
buf_fill(pxl_t color) {
	pxl_t *dst = g_buf.data;
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			dst[x] = color;
		}
		dst += g_buf.stride;
	}
}

/* Tests ------------------------------------------------------------------- */

/* --- View Tests ---------------------------------------------------------- */

static void
test_canvas_view(void) {
	pxl_canvas_t cnv;
	
	/* Test with full view */
	pxl_canvas_init(&cnv, &g_buf);
    pxl_rect_t vp = pxl_canvas_view(&cnv);
	assert(vp.x == 0);
	assert(vp.y == 0);
	assert(vp.w == FIXTURE_W);
	assert(vp.h == FIXTURE_H);
	
	/* Test with custom scissor */
	pxl_canvas_set_scissor(&cnv, 10, 20, 80, 60);
    vp = pxl_canvas_view(&cnv);
	assert(vp.x == 0);
	assert(vp.y == 0);
	assert(vp.w == 80);
	assert(vp.h == 60);
}

static void
test_canvas_init_view_basic(void) {
	pxl_canvas_t cnv;
	
	pxl_canvas_init_view(&cnv, &g_buf, (pxl_rect_t){10, 20, 80, 60});
	
	assert(cnv.pb == &g_buf);
	assert(cnv.offset_x == 10 && cnv.offset_y == 20);
	assert(cnv.scissor.x == 10 && cnv.scissor.y == 20);
	assert(cnv.scissor.w == 80 && cnv.scissor.h == 60);
	assert(cnv.color == 0xFFFFFFFF);
}

static void
test_canvas_init_view_at_origin(void) {
	pxl_canvas_t cnv;
	
	pxl_canvas_init_view(&cnv, &g_buf, (pxl_rect_t){0, 0, FIXTURE_W, FIXTURE_H});
	
	assert(cnv.offset_x == 0 && cnv.offset_y == 0);
	assert(cnv.scissor.x == 0 && cnv.scissor.y == 0);
	assert(cnv.scissor.w == FIXTURE_W && cnv.scissor.h == FIXTURE_H);
}

static void
test_canvas_init_view_clipped(void) {
	pxl_canvas_t cnv;
	
	pxl_canvas_init_view(&cnv, &g_buf, (pxl_rect_t){50, 50, 200, 200});
	
	assert(cnv.offset_x == 50 && cnv.offset_y == 50);
	assert(cnv.scissor.x == 50 && cnv.scissor.y == 50);
	assert(cnv.scissor.w == 51 && cnv.scissor.h == 78);
}


/* --- Core Canvas Tests --------------------------------------------------- */

static void
test_canvas_init(void) {
	fixture_reset();
	
	assert(g_cnv.pb == &g_buf);
	assert(g_cnv.color == 0xFFFFFFFF);
	assert(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	assert(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
	assert(g_cnv.offset_x == 0 && g_cnv.offset_y == 0);
}

static void
test_canvas_set_color(void) {
	fixture_reset();
	
	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	assert(g_cnv.color == 0xFF00FF00);
}

static void
test_canvas_set_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 10, 20, 30, 40);
	assert(g_cnv.scissor.x == 10 && g_cnv.scissor.y == 20);
	assert(g_cnv.scissor.w == 30 && g_cnv.scissor.h == 40);
}

static void
test_canvas_set_scissor_clipped(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, -10, -10, 200, 200);
	assert(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	assert(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
}

static void
test_canvas_set_scissor_zero(void) {
	fixture_reset();
	
	/* Zero width and height are valid (w >= 0, h >= 0) */
	pxl_canvas_set_scissor(&g_cnv, 0, 0, 0, 0);
	assert(g_cnv.scissor.w == 0 && g_cnv.scissor.h == 0);
	
	/* clear() with empty scissor must do nothing */
	buf_fill(0xFF0000FF);
	pxl_canvas_set_color(&g_cnv, 0xFFFFFFFF);
	pxl_canvas_clear(&g_cnv);
	/* Buffer unchanged */
	assert(*pxl_buf_ptr(&g_buf, 0, 0) == 0xFF0000FF);
}

static void
test_canvas_reset_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 10, 10, 20, 20);
	assert(g_cnv.scissor.x == 10);
	
	pxl_canvas_reset_scissor(&g_cnv);
	assert(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	assert(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
}

static void
test_canvas_set_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 10, 20);
	assert(g_cnv.offset_x == 10 && g_cnv.offset_y == 20);
	
	pxl_canvas_set_offset(&g_cnv, -5, -3);
	assert(g_cnv.offset_x == -5 && g_cnv.offset_y == -3);
}

static void
test_canvas_reset_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 15, 25);
	assert(g_cnv.offset_x == 15 && g_cnv.offset_y == 25);
	
	pxl_canvas_reset_offset(&g_cnv);
	assert(g_cnv.offset_x == 0 && g_cnv.offset_y == 0);
}

static void
test_canvas_clear_full(void) {
	fixture_reset();
	
	pxl_canvas_clear(&g_cnv);
	
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			assert(*pxl_buf_ptr(&g_buf, x, y) == 0xFFFFFFFF);
		}
	}
}

static void
test_canvas_clear_with_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 2, 2, 6, 6);
	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	pxl_canvas_clear(&g_cnv);
	
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_scissor(x, y) ? 0xFF00FF00 : 0x00;
			assert(got == want);
		}
	}
}

static void
test_canvas_clear_fast_path_black(void) {
	fixture_reset();
	
	buf_fill(0xFFFFFFFF);
	pxl_canvas_set_color(&g_cnv, 0x00);
	pxl_canvas_clear(&g_cnv);
	
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			assert(*pxl_buf_ptr(&g_buf, x, y) == 0x00);
		}
	}
}

static void
test_canvas_clear_fast_path_white(void) {
	fixture_reset();
	
	buf_fill(0xFF0000FF);
	pxl_canvas_set_color(&g_cnv, 0xFFFFFFFF);
	pxl_canvas_clear(&g_cnv);
	
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			assert(*pxl_buf_ptr(&g_buf, x, y) == 0xFFFFFFFF);
		}
	}
}

/* Main ------------------------------------------------------------------- */

int
main(void) {
	/* View tests */
	test_canvas_view();
	test_canvas_init_view_basic();
	test_canvas_init_view_at_origin();
	test_canvas_init_view_clipped();
	
	/* Core canvas tests */
	test_canvas_init();
	test_canvas_set_color();
	test_canvas_set_scissor();
	test_canvas_set_scissor_clipped();
	test_canvas_set_scissor_zero();
	test_canvas_reset_scissor();
	test_canvas_set_offset();
	test_canvas_reset_offset();
	test_canvas_clear_full();
	test_canvas_clear_with_scissor();
	test_canvas_clear_fast_path_black();
	test_canvas_clear_fast_path_white();
	
	return 0;
}
