#include <string.h>
#include "test.h"
#include "bitmask.h"
#include "canvas.h"
#include "buf.h"
#include "blit.h"
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

/* Color constants */
#define COLOR_WHITE  0xFFFFFFFFU
#define COLOR_RED    0xFFFF0000U
#define COLOR_GREEN  0xFF00FF00U
#define COLOR_BLUE   0xFF0000FFU
#define COLOR_YELLOW 0xFFFFFF00U

/* Helpers ----------------------------------------------------------------- */

static inline bool
is_inside_scissor(int x, int y) {
	const pxl_rect_t *s = &g_cnv.scissor;
	return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

static inline bool
is_drawn_inside_rect(int x, int y, int rx, int ry, int rw, int rh) {
	rx += g_cnv.offset_x;
	ry += g_cnv.offset_y;

	return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

/* Bitmask helpers ----------------------------------------------------------- */

static inline uint32_t
bitmask_get(const pxl_bitmask_t *bm, int x, int y) {
	assert(x >= 0 && x < bm->width);
	assert(y >= 0 && y < bm->height);
	
	size_t bit_index = (size_t)y * ((size_t)bm->stride << 3) + (size_t)x;
	size_t byte_index = bit_index >> 3;
	unsigned bit_offset = bit_index & 0x7;
	
	uint8_t byte = bm->data[byte_index];
	return (uint32_t)((byte >> bit_offset) & 0x1U);
}

static inline bool
is_drawn_on_bitmask(int x, int y, const pxl_bitmask_t *bm, pxl_rect_t bm_r, int cnv_x, int cnv_y) {
	cnv_x += g_cnv.offset_x;
	cnv_y += g_cnv.offset_y;

	int bm_x = bm_r.x + x - cnv_x;
	int bm_y = bm_r.y + y - cnv_y;

	if (bm_x < bm_r.x || bm_x >= bm_r.x + bm_r.w ||
			bm_y < bm_r.y || bm_y >= bm_r.y + bm_r.h) {
		return false;
	}
	
	return bitmask_get(bm, bm_x, bm_y) == 1;
}

/* Static source buffers for blit tests */
#define SRC_10_STRIDE 12   /* pxl_calc_stride(10) = 12 */
#define SRC_15_STRIDE 16   /* pxl_calc_stride(15) = 16 */
#define SRC_8_STRIDE 8     /* pxl_calc_stride(8) = 8 */

static pxl_t g_src_pb_10x10_data[SRC_10_STRIDE * 10];
static pxl_buf_t g_src_pb_10x10 = {
	.data = g_src_pb_10x10_data,
	.width = 10,
	.height = 10,
	.stride = SRC_10_STRIDE
};

static pxl_t g_src_pb_15x15_data[SRC_15_STRIDE * 15];
static pxl_buf_t g_src_pb_15x15 = {
	.data = g_src_pb_15x15_data,
	.width = 15,
	.height = 15,
	.stride = SRC_15_STRIDE
};

static pxl_t g_src_pb_8x8_data[SRC_8_STRIDE * 8];
static pxl_buf_t g_src_pb_8x8 = {
	.data = g_src_pb_8x8_data,
	.width = 8,
	.height = 8,
	.stride = SRC_8_STRIDE
};

static inline void
src_buf_fill(pxl_buf_t *pb, pxl_t color) {
	for (int y = 0; y < pb->height; y++) {
		pxl_t *row = pxl_buf_ptr(pb, 0, y);
		for (int x = 0; x < pb->width; x++) {
			row[x] = color;
		}
	}
}

/* Static bitmasks for bitmask tests */
static uint8_t g_bm_checkerboard_data[1] = { 0xAA };
static pxl_bitmask_t g_bm_checkerboard = {
	.data = g_bm_checkerboard_data,
	.width = 8,
	.height = 2,
	.stride = 1
};

static uint8_t g_bm_all_set_16x1_data[2] = { 0xFF, 0xFF };
static pxl_bitmask_t g_bm_all_set_16x1 = {
	.data = g_bm_all_set_16x1_data,
	.width = 16,
	.height = 1,
	.stride = 2
};

static uint8_t g_bm_all_set_16x8_data[16] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
static pxl_bitmask_t g_bm_all_set_16x8 = {
	.data = g_bm_all_set_16x8_data,
	.width = 16,
	.height = 8,
	.stride = 2
};

static uint8_t g_bm_all_set_8x1_data[1] = { 0xFF };
static pxl_bitmask_t g_bm_all_set_8x1 = {
	.data = g_bm_all_set_8x1_data,
	.width = 8,
	.height = 1,
	.stride = 1
};

/* Tests - Blit ---------------------------------------------------------------- */

static void
test_pxl_blit_rect_basic(void) {
	fixture_reset();
	
	pxl_t color = COLOR_GREEN;
	src_buf_fill(&g_src_pb_10x10, color);
	
	pxl_rect_t pb_r = {0, 0, 10, 10};
	int cnv_x = 5, cnv_y = 5;
	pxl_blit_rect(&g_cnv, &g_src_pb_10x10, pb_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_blit_rect_with_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_GREEN;
	src_buf_fill(&g_src_pb_15x15, color);
	
	pxl_rect_t pb_r = {0, 0, 15, 15};
	int cnv_x = 0, cnv_y = 0;
	pxl_blit_rect(&g_cnv, &g_src_pb_15x15, pb_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_blit_rect_fully_clipped(void) {
	fixture_reset();
	
	pxl_t color = COLOR_BLUE;
	src_buf_fill(&g_src_pb_10x10, color);
	
	pxl_rect_t pb_r = {0, 0, 10, 10};
	int cnv_x = 30, cnv_y = 30;
	pxl_blit_rect(&g_cnv, &g_src_pb_10x10, pb_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_blit_rect_with_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 5, 5);
	
	pxl_t color = COLOR_YELLOW;
	src_buf_fill(&g_src_pb_8x8, color);
	
	pxl_rect_t pb_r = {0, 0, 8, 8};
	int cnv_x = 0, cnv_y = 0;
	pxl_blit_rect(&g_cnv, &g_src_pb_8x8, pb_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_blit_rect_partially_clipped(void) {
	fixture_reset();
	
	pxl_t color = COLOR_YELLOW;
	src_buf_fill(&g_src_pb_15x15, color);
	
	pxl_rect_t pb_r = {0, 0, 15, 15};
	int cnv_x = -5, cnv_y = -5;
	pxl_blit_rect(&g_cnv, &g_src_pb_15x15, pb_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_blit = is_drawn_inside_rect(x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

/* Tests - Bitmask draw --------------------------------------------------------- */

static void
test_pxl_draw_bitmask_basic(void) {
	fixture_reset();
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	pxl_rect_t bm_r = {0, 0, 8, 2};
	int cnv_x = 2, cnv_y = 2;
	pxl_draw_bitmask(&g_cnv, &g_bm_checkerboard, bm_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_checkerboard, bm_r, cnv_x, cnv_y);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_bitmask_all_bits_set(void) {
	fixture_reset();
	
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&g_cnv, color);
	
	pxl_rect_t bm_r = {0, 0, 16, 1};
	int cnv_x = 2, cnv_y = 2;
	pxl_draw_bitmask(&g_cnv, &g_bm_all_set_16x1, bm_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 32; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_16x1, bm_r, cnv_x, cnv_y);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_bitmask_clipped(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 4, 4, 8, 8);
	
	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&g_cnv, color);
	
	pxl_rect_t bm_r = {0, 0, 16, 8};
	int cnv_x = 0, cnv_y = 0;
	pxl_draw_bitmask(&g_cnv, &g_bm_all_set_16x8, bm_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 32; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_16x8, bm_r, cnv_x, cnv_y);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_bitmask_with_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 5, 5);
	
	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&g_cnv, color);
	
	pxl_rect_t bm_r = {0, 0, 8, 1};
	int cnv_x = 0, cnv_y = 0;
	pxl_draw_bitmask(&g_cnv, &g_bm_all_set_8x1, bm_r, cnv_x, cnv_y);
	
	for (int y = 0; y < 16; ++y) {
		for (int x = 0; x < 16; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_bitmask = is_drawn_on_bitmask(x, y, &g_bm_all_set_8x1, bm_r, cnv_x, cnv_y);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

/* Main ----------------------------------------------------------------------- */
int
main(void) {
	/* Blit tests */
	test_pxl_blit_rect_basic();
	test_pxl_blit_rect_with_scissor();
	test_pxl_blit_rect_fully_clipped();
	test_pxl_blit_rect_with_offset();
	test_pxl_blit_rect_partially_clipped();

	/* Bitmask draw tests */
	test_pxl_draw_bitmask_basic();
	test_pxl_draw_bitmask_all_bits_set();
	test_pxl_draw_bitmask_clipped();
	test_pxl_draw_bitmask_with_offset();

	return 0;
}
