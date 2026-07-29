#include <string.h>
#include "test.h"
#include "canvas.h"
#include "buf.h"

/* Fixture ----------------------------------------------------------------- */
#define FIXTURE_W 101
#define FIXTURE_H 128
#define FIXTURE_STRIDE pxl_calc_stride(FIXTURE_W)

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

static void
test_canvas_init(void) {
	fixture_reset();
	
	ASSERT(g_cnv.pb == &g_buf);
	ASSERT(g_cnv.color == 0xFFFFFFFF);
	ASSERT(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	ASSERT(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
	ASSERT(g_cnv.offset_x == 0 && g_cnv.offset_y == 0);
}

static void
test_canvas_set_color(void) {
	fixture_reset();
	
	pxl_canvas_set_color(&g_cnv, 0xFF00FF00);
	ASSERT(g_cnv.color == 0xFF00FF00);
}

static void
test_canvas_set_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 10, 20, 30, 40);
	ASSERT(g_cnv.scissor.x == 10 && g_cnv.scissor.y == 20);
	ASSERT(g_cnv.scissor.w == 30 && g_cnv.scissor.h == 40);
}

static void
test_canvas_set_scissor_clipped(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, -10, -10, 200, 200);
	ASSERT(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	ASSERT(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
}

static void
test_canvas_reset_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 10, 10, 20, 20);
	ASSERT(g_cnv.scissor.x == 10);
	
	pxl_canvas_reset_scissor(&g_cnv);
	ASSERT(g_cnv.scissor.x == 0 && g_cnv.scissor.y == 0);
	ASSERT(g_cnv.scissor.w == FIXTURE_W && g_cnv.scissor.h == FIXTURE_H);
}

static void
test_canvas_set_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 10, 20);
	ASSERT(g_cnv.offset_x == 10 && g_cnv.offset_y == 20);
	
	pxl_canvas_set_offset(&g_cnv, -5, -3);
	ASSERT(g_cnv.offset_x == -5 && g_cnv.offset_y == -3);
}

static void
test_canvas_reset_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 15, 25);
	ASSERT(g_cnv.offset_x == 15 && g_cnv.offset_y == 25);
	
	pxl_canvas_reset_offset(&g_cnv);
	ASSERT(g_cnv.offset_x == 0 && g_cnv.offset_y == 0);
}

static void
test_canvas_clear_full(void) {
	fixture_reset();
	
	pxl_canvas_clear(&g_cnv);
	
	for (int y = 0; y < g_buf.height; ++y) {
		for (int x = 0; x < g_buf.width; ++x) {
			ASSERT(*pxl_buf_ptr(&g_buf, x, y) == 0xFFFFFFFF);
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
			ASSERT(got == want);
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
			ASSERT(*pxl_buf_ptr(&g_buf, x, y) == 0x00);
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
			ASSERT(*pxl_buf_ptr(&g_buf, x, y) == 0xFFFFFFFF);
		}
	}
}

/* Main ------------------------------------------------------------------- */

int
main(void) {
	test_canvas_init();
	test_canvas_set_color();
	test_canvas_set_scissor();
	test_canvas_set_scissor_clipped();
	test_canvas_reset_scissor();
	test_canvas_set_offset();
	test_canvas_reset_offset();
	test_canvas_clear_full();
	test_canvas_clear_with_scissor();
	test_canvas_clear_fast_path_black();
	test_canvas_clear_fast_path_white();
	
	return 0;
}
