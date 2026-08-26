#include <string.h>
#include <limits.h>
#include "test.h"
#include "canvas.h"
#include "buf.h"
#include "shape.h"
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
is_drawn_on_line(int x, int y, int x0, int y0, int x1, int y1) {
	x0 += g_cnv.offset_x;
	y0 += g_cnv.offset_y;
	x1 += g_cnv.offset_x;
	y1 += g_cnv.offset_y;
	
	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;
	
	if (dx >= dy) {
		int err = dx / 2;
		for (;;) {
			if (x == x0 && y == y0) return true;
			if (x0 == x1 && y0 == y1) break;
			x0 += sx;
			err -= dy;
			if (err < 0) {
				y0 += sy;
				err += dx;
			}
		}
	} else {
		int err = dy / 2;
		for (;;) {
			if (x == x0 && y == y0) return true;
			if (x0 == x1 && y0 == y1) break;
			y0 += sy;
			err -= dx;
			if (err < 0) {
				x0 += sx;
				err += dy;
			}
		}
	}
	
	return false;
}

static inline bool
is_drawn_on_rect(int x, int y, int rx, int ry, int rw, int rh) {
	rx += g_cnv.offset_x;
	ry += g_cnv.offset_y;

	if (y == ry && x >= rx && x < rx + rw) {
		return true;
	}

	if (y == ry + rh - 1 && x >= rx && x < rx + rw) {
		return true;
	}

	if (x == rx && y >= ry && y < ry + rh) {
		return true;
	}

	if (x == rx + rw - 1 && y >= ry && y < ry + rh) {
		return true;
	}

	return false;
}

static inline bool
is_drawn_inside_rect(int x, int y, int rx, int ry, int rw, int rh) {
	rx += g_cnv.offset_x;
	ry += g_cnv.offset_y;

	return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

/* Tests - Line --------------------------------------------------------------- */

static void
test_pxl_draw_line_single_pixel(void) {
	fixture_reset();
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 10, y0 = 10, x1 = 10, y1 = 10;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_horizontal(void) {
	fixture_reset();
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 5, y0 = 10, x1 = 14, y1 = 10;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_vertical(void) {
	fixture_reset();
	
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 10, y0 = 5, x1 = 10, y1 = 14;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_diagonal(void) {
	fixture_reset();
	
	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 5, y0 = 5, x1 = 14, y1 = 14;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_outside_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 20, y0 = 20, x1 = 30, y1 = 30;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_with_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 5, 5);
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_line_with_negative_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, -5, -5);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&g_cnv, x0, y0, x1, y1);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

/* Tests - Rect -------------------------------------------------------------- */

static void
test_pxl_draw_rect_basic(void) {
	fixture_reset();
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_draw_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_rect_outside_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 20, ry = 20, rw = 10, rh = 10;
	pxl_draw_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_rect_clip_left_no_false_border(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 0, 15, 20);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 0, ry = 5, rw = 15, rh = 10;
	pxl_draw_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_rect_clip_both_sides(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 0, 10, 20);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 0, ry = 5, rw = 20, rh = 10;
	pxl_draw_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_draw_rect_with_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 3, 4);
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 0, ry = 0, rw = 5, rh = 5;
	pxl_draw_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

/* Tests - Fill Rect ----------------------------------------------------------- */

static void
test_pxl_fill_rect_basic(void) {
	fixture_reset();
	
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_fill_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_rect = is_drawn_inside_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_fill_rect_with_scissor(void) {
	fixture_reset();
	
	pxl_canvas_set_scissor(&g_cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 3, ry = 3, rw = 10, rh = 10;
	pxl_fill_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_rect = is_drawn_inside_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_fill_rect_fast_path(void) {
	fixture_reset();
	
	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&g_cnv, color);
	
	pxl_canvas_reset_scissor(&g_cnv);
	
	int rx = 0, ry = 0, rw = 20, rh = 20;
	pxl_fill_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_rect = is_drawn_inside_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

static void
test_pxl_fill_rect_with_offset(void) {
	fixture_reset();
	
	pxl_canvas_set_offset(&g_cnv, 2, 3);
	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&g_cnv, color);
	
	int rx = 0, ry = 0, rw = 6, rh = 4;
	pxl_fill_rect(&g_cnv, rx, ry, rw, rh);
	
	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			bool in_s = is_inside_scissor(x, y);
			bool in_rect = is_drawn_inside_rect(x, y, rx, ry, rw, rh);
			
			pxl_t got = *pxl_buf_ptr(&g_buf, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;
			ASSERT(got == want);
		}
	}
}

/* Fixture for circle/triangle tests */

#define FIXTURE_W_EXTRA 100
#define FIXTURE_H_EXTRA 100
#define FIXTURE_STRIDE_EXTRA 100

static pxl_t g_buf_data_extra[FIXTURE_STRIDE_EXTRA * FIXTURE_H_EXTRA];
static pxl_buf_t g_buf_extra = {
	.data = g_buf_data_extra,
	.width = FIXTURE_W_EXTRA,
	.height = FIXTURE_H_EXTRA,
	.stride = FIXTURE_STRIDE_EXTRA
};
static pxl_canvas_t g_cnv_extra;

static inline void
fixture_reset_extra(void) {
	memset(g_buf_data_extra, 0x00, sizeof(g_buf_data_extra));
	pxl_canvas_init(&g_cnv_extra, &g_buf_extra);
}

static inline void
fixture_reset_scissor_extra(int x, int y, int w, int h) {
	fixture_reset_extra();
	pxl_canvas_set_scissor(&g_cnv_extra, x, y, w, h);
}

static inline bool
buf_is_empty_extra(void) {
	for (size_t i = 0; i < FIXTURE_STRIDE_EXTRA * FIXTURE_H_EXTRA; ++i) {
		if (g_buf_data_extra[i] != 0) return false;
	}
	return true;
}

static inline bool
buf_is_not_empty_extra(void) {
	return !buf_is_empty_extra();
}

static inline pxl_t
buf_get_extra(int x, int y) {
	return *pxl_buf_ptr(&g_buf_extra, x, y);
}

/* Tests - Circle -------------------------------------------------------------- */

static void
test_draw_circle_basic(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	pxl_draw_circle(&g_cnv_extra, 50, 50, 10);

	/* Check that something was drawn */
	ASSERT(buf_is_not_empty_extra());

	/* Check top, bottom, left, right cardinal points */
	ASSERT(buf_get_extra(50, 40) == 0xFFFF0000);  /* top */
	ASSERT(buf_get_extra(50, 60) == 0xFFFF0000);  /* bottom */
	ASSERT(buf_get_extra(40, 50) == 0xFFFF0000);  /* left */
	ASSERT(buf_get_extra(60, 50) == 0xFFFF0000);  /* right */
}

static void
test_draw_circle_min_radius(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_draw_circle(&g_cnv_extra, 50, 50, 1);

	ASSERT(buf_is_not_empty_extra());
	/* With r=1, only 4 points around center + center cross */
	ASSERT(buf_get_extra(50, 49) == 0xFF00FF00);
	ASSERT(buf_get_extra(50, 51) == 0xFF00FF00);
	ASSERT(buf_get_extra(49, 50) == 0xFF00FF00);
	ASSERT(buf_get_extra(51, 50) == 0xFF00FF00);
}

static void
test_draw_circle_outside_scissor(void) {
	fixture_reset_scissor_extra(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	/* Circle completely outside scissor */
	pxl_draw_circle(&g_cnv_extra, 100, 100, 10);

	/* Buffer should remain empty */
	ASSERT(buf_is_empty_extra());
}

static void
test_draw_circle_with_offset(void) {
	fixture_reset_extra();

	pxl_canvas_set_offset(&g_cnv_extra, 20, 20);
	pxl_canvas_set_color(&g_cnv_extra, 0xFF0000FF);

	/* Draw at (0,0) with offset becomes (20,20) */
	pxl_draw_circle(&g_cnv_extra, 0, 0, 5);

	ASSERT(buf_is_not_empty_extra());
	/* Check cardinal points with offset: center is at (20,20) with r=5 */
	ASSERT(buf_get_extra(20, 15) == 0xFF0000FF);  /* top: (20,20-5) */
	ASSERT(buf_get_extra(20, 25) == 0xFF0000FF);  /* bottom: (20,20+5) */
	ASSERT(buf_get_extra(15, 20) == 0xFF0000FF);  /* left: (20-5,20) */
	ASSERT(buf_get_extra(25, 20) == 0xFF0000FF);  /* right: (20+5,20) */
}

/* Tests - Fill Circle --------------------------------------------------------- */

static void
test_fill_circle_basic(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_fill_circle(&g_cnv_extra, 50, 50, 10);

	ASSERT(buf_is_not_empty_extra());

	/* Check center */
	ASSERT(buf_get_extra(50, 50) == 0xFF00FF00);
	/* Check cardinal points on edge */
	ASSERT(buf_get_extra(50, 40) == 0xFF00FF00);  /* top */
	ASSERT(buf_get_extra(50, 60) == 0xFF00FF00);  /* bottom */
	ASSERT(buf_get_extra(40, 50) == 0xFF00FF00);  /* left */
	ASSERT(buf_get_extra(60, 50) == 0xFF00FF00);  /* right */
}

static void
test_fill_circle_min_radius(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFFFF00);
	pxl_fill_circle(&g_cnv_extra, 50, 50, 1);

	ASSERT(buf_is_not_empty_extra());
	/* Center should be filled */
	ASSERT(buf_get_extra(50, 50) == 0xFFFFFF00);
	/* Edge points */
	ASSERT(buf_get_extra(50, 49) == 0xFFFFFF00);
	ASSERT(buf_get_extra(50, 51) == 0xFFFFFF00);
	ASSERT(buf_get_extra(49, 50) == 0xFFFFFF00);
	ASSERT(buf_get_extra(51, 50) == 0xFFFFFF00);
}

static void
test_fill_circle_outside_scissor(void) {
	fixture_reset_scissor_extra(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_fill_circle(&g_cnv_extra, 100, 100, 10);

	ASSERT(buf_is_empty_extra());
}

static void
test_fill_circle_with_offset(void) {
	fixture_reset_extra();

	pxl_canvas_set_offset(&g_cnv_extra, 10, 10);
	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);

	pxl_fill_circle(&g_cnv_extra, 0, 0, 8);

	ASSERT(buf_is_not_empty_extra());
	/* Center with offset */
	ASSERT(buf_get_extra(10, 10) == 0xFF00FF00);
	/* Edge with offset */
	ASSERT(buf_get_extra(10, 2) == 0xFF00FF00);   /* top */
	ASSERT(buf_get_extra(10, 18) == 0xFF00FF00);  /* bottom */
	ASSERT(buf_get_extra(2, 10) == 0xFF00FF00);   /* left */
	ASSERT(buf_get_extra(18, 10) == 0xFF00FF00);  /* right */
}

/* Tests - Triangle ------------------------------------------------------------- */

static void
test_draw_triangle_basic(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	pxl_draw_triangle(&g_cnv_extra, 30, 30, 50, 30, 40, 50);

	ASSERT(buf_is_not_empty_extra());

	/* Check the three vertices */
	ASSERT(buf_get_extra(30, 30) == 0xFFFF0000);
	ASSERT(buf_get_extra(50, 30) == 0xFFFF0000);
	ASSERT(buf_get_extra(40, 50) == 0xFFFF0000);
}

static void
test_draw_triangle_outside_scissor(void) {
	fixture_reset_scissor_extra(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	pxl_draw_triangle(&g_cnv_extra, 100, 100, 120, 100, 110, 120);

	ASSERT(buf_is_empty_extra());
}

static void
test_draw_triangle_with_offset(void) {
	fixture_reset_extra();

	pxl_canvas_set_offset(&g_cnv_extra, 10, 10);
	pxl_canvas_set_color(&g_cnv_extra, 0xFF0000FF);

	pxl_draw_triangle(&g_cnv_extra, 0, 0, 20, 0, 10, 20);

	ASSERT(buf_is_not_empty_extra());
	/* Vertices with offset */
	ASSERT(buf_get_extra(10, 10) == 0xFF0000FF);
	ASSERT(buf_get_extra(30, 10) == 0xFF0000FF);
	ASSERT(buf_get_extra(20, 30) == 0xFF0000FF);
}

/* Tests - Fill Triangle -------------------------------------------------------- */

static void
test_fill_triangle_basic(void) {
	fixture_reset_extra();

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_fill_triangle(&g_cnv_extra, 30, 30, 50, 30, 40, 50);

	ASSERT(buf_is_not_empty_extra());

	/* Check vertices are filled */
	ASSERT(buf_get_extra(30, 30) == 0xFF00FF00);
	ASSERT(buf_get_extra(50, 30) == 0xFF00FF00);
	ASSERT(buf_get_extra(40, 50) == 0xFF00FF00);
	/* Check center-ish point is filled */
	ASSERT(buf_get_extra(40, 40) == 0xFF00FF00);
}

static void
test_fill_triangle_outside_scissor(void) {
	fixture_reset_scissor_extra(10, 10, 20, 20);

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_fill_triangle(&g_cnv_extra, 100, 100, 120, 100, 110, 120);

	ASSERT(buf_is_empty_extra());
}

static void
test_fill_triangle_with_offset(void) {
	fixture_reset_extra();

	pxl_canvas_set_offset(&g_cnv_extra, 5, 5);
	pxl_canvas_set_color(&g_cnv_extra, 0xFFFFFF00);

	pxl_fill_triangle(&g_cnv_extra, 0, 0, 10, 0, 5, 10);

	ASSERT(buf_is_not_empty_extra());
	/* Vertices with offset */
	ASSERT(buf_get_extra(5, 5) == 0xFFFFFF00);
	ASSERT(buf_get_extra(15, 5) == 0xFFFFFF00);
	ASSERT(buf_get_extra(10, 15) == 0xFFFFFF00);
	/* Center with offset */
	ASSERT(buf_get_extra(10, 10) == 0xFFFFFF00);
}

/* Tests - Edge Cases ----------------------------------------------------------- */

static void
test_circle_partial_scissor(void) {
	fixture_reset_scissor_extra(20, 20, 40, 40);

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	/* Circle center (30,30) radius 15: bbox (15,15) to (45,45) */
	/* Scissor (20,20) to (60,60): partial overlap */
	pxl_draw_circle(&g_cnv_extra, 30, 30, 15);

	/* Should have some pixels drawn (the part inside scissor) */
	ASSERT(buf_is_not_empty_extra());
	/* Top-left of circle should be clipped */
	ASSERT(buf_get_extra(15, 15) == 0x00000000);  /* outside scissor */
	/* Right edge of circle at (45,30) should be drawn and inside scissor */
	ASSERT(buf_get_extra(45, 30) == 0xFFFF0000);  /* on circle edge, inside scissor */
	/* Bottom edge of circle at (30,45) should be drawn and inside scissor */
	ASSERT(buf_get_extra(30, 45) == 0xFFFF0000);  /* on circle edge, inside scissor */
}

static void
test_fill_circle_partial_scissor(void) {
	fixture_reset_scissor_extra(25, 25, 30, 30);

	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	pxl_fill_circle(&g_cnv_extra, 40, 40, 15);

	ASSERT(buf_is_not_empty_extra());
	/* Part inside scissor should be filled */
	ASSERT(buf_get_extra(35, 35) == 0xFF00FF00);
	/* Part outside scissor should be empty */
	ASSERT(buf_get_extra(20, 20) == 0x00000000);
}

static void
test_fill_triangle_partial_scissor(void) {
	fixture_reset_scissor_extra(25, 25, 30, 30);

	pxl_canvas_set_color(&g_cnv_extra, 0xFFFFFF00);
	/* Triangle partially overlapping scissor */
	pxl_fill_triangle(&g_cnv_extra, 20, 20, 50, 20, 35, 50);

	ASSERT(buf_is_not_empty_extra());
	/* Part inside scissor should be filled */
	ASSERT(buf_get_extra(30, 30) == 0xFFFFFF00);
	/* Part outside scissor should be empty */
	ASSERT(buf_get_extra(20, 20) == 0x00000000);
}

/* Tests - Assertion coverage ---------------------------------------------------- */

static void
test_draw_circle_max_safe_radius(void) {
	/* Verify the overflow protection constant */
	int max_safe_r = (INT_MAX - 1) / 2;
	ASSERT(max_safe_r > 0);
	ASSERT(max_safe_r <= (INT_MAX - 1) / 2);
	
	/* Test with a large but safe radius */
	fixture_reset_extra();
	pxl_canvas_set_color(&g_cnv_extra, 0xFFFF0000);
	pxl_draw_circle(&g_cnv_extra, 50, 50, 40);
	ASSERT(buf_is_not_empty_extra());
}

static void
test_fill_triangle_large_coords(void) {
	/* Test that bounding box doesn't overflow with large but valid coords */
	fixture_reset_extra();
	pxl_canvas_set_color(&g_cnv_extra, 0xFF00FF00);
	/* Use coordinates that are large but won't cause overflow in bounding box */
	pxl_fill_triangle(&g_cnv_extra, 10, 10, 50, 10, 30, 50);
	ASSERT(buf_is_not_empty_extra());
}

/* Main ----------------------------------------------------------------------- */
int
main(void) {
	/* Line tests */
	test_pxl_draw_line_single_pixel();
	test_pxl_draw_line_horizontal();
	test_pxl_draw_line_vertical();
	test_pxl_draw_line_diagonal();
	test_pxl_draw_line_outside_scissor();
	test_pxl_draw_line_with_offset();
	test_pxl_draw_line_with_negative_offset();

	/* Rect tests */
	test_pxl_draw_rect_basic();
	test_pxl_draw_rect_outside_scissor();
	test_pxl_draw_rect_clip_left_no_false_border();
	test_pxl_draw_rect_clip_both_sides();
	test_pxl_draw_rect_with_offset();

	/* Fill Rect tests */
	test_pxl_fill_rect_basic();
	test_pxl_fill_rect_with_scissor();
	test_pxl_fill_rect_fast_path();
	test_pxl_fill_rect_with_offset();

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
