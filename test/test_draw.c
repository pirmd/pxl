#include <string.h>
#include "bitmask.h"
#include "canvas.h"
#include "buf.h"
#include "draw.h"
#include "geom.h"
#include "stest/stest.h"

/* Fixture ----------------------------------------------------------------- */

#define COLOR_WHITE  0xFFFFFFFFU  /* Opaque white */
#define COLOR_RED    0xFFFF0000U  /* Opaque red */
#define COLOR_GREEN  0xFF00FF00U  /* Opaque green */
#define COLOR_BLUE   0xFF0000FFU  /* Opaque blue */
#define COLOR_YELLOW 0xFFFFFF00U  /* Opaque yellow */

typedef struct {
	pxl_buf_t pb;
	pxl_canvas_t cnv;
} fixture_t;

static void
pxl_buf_zero(pxl_buf_t *pb) {
	assert(pb && pb->data);
	memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pxl_t));
}

static void
pxl_buf_fill(pxl_buf_t *pb, pxl_t color) {
	for (int y = 0; y < pb->height; y++) {
		pxl_t *row = pxl_buf_ptr(pb, 0, y);
		for (int x = 0; x < pb->width; x++) {
			row[x] = color;
		}
	}
}

static bool
fixture_init(const st_ctx_t *ctx, fixture_t *f, int w, int h) {
	f->pb.width = w;
	f->pb.height = h;
	f->pb.stride = pxl_calc_stride(w);
	f->pb.data = malloc(f->pb.stride * f->pb.height * sizeof(pxl_t));

	if (!st_check(ctx, f->pb.data != NULL, "malloc failed")) {
		return false;
	}

	pxl_canvas_init(&f->cnv, &f->pb);
	pxl_buf_zero(&f->pb);
	
	return true;
}

static void
fixture_deinit(fixture_t *f) {
	free(f->pb.data);
	f->pb.data = NULL;
}

/* Helpers ----------------------------------------------------------------- */
static inline bool
is_inside_scissor(const pxl_canvas_t *cnv, int x, int y) {
	return pxl_in_rect(x, y, cnv->scissor);
}

/* Test Line -------------------------------------------------------------- */
static inline bool
is_drawn_on_line(const pxl_canvas_t *cnv, int x, int y, int x0, int y0, int x1, int y1) {
	x0 += cnv->offset_x;
	y0 += cnv->offset_y;
	x1 += cnv->offset_x;
	y1 += cnv->offset_y;
	
	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {  /* X-major line */
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
	} else {  /* Y-major line */
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

static void
test_pxl_draw_line_horizontal(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Horizontal line at y=10, x=5 to 14 */
	int x0 = 5, y0 = 10, x1 = 14, y1 = 10;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_vertical(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Vertical line at x=10, y=5 to 14 */
	int x0 = 10, y0 = 5, x1 = 10, y1 = 14;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_diagonal(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&f.cnv, color);

	/* Diagonal line from (5,5) to (14,14) */
	int x0 = 5, y0 = 5, x1 = 14, y1 = 14;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int x0 = 20, y0 = 20, x1 = 30, y1 = 30;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 5, 5);
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_with_offset_and_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 10, 10);
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&f.cnv, color);

	int x0 = 0, y0 = 0, x1 = 5, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_line_with_negative_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, -5, -5);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool    in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
		
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_line ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Rect -------------------------------------------------------------- */
static inline bool
is_drawn_on_rect(const pxl_canvas_t *cnv, int x, int y, int rx, int ry, int rw, int rh) {
	assert(rw > 0 && rh > 0);

	rx += cnv->offset_x;
	ry += cnv->offset_y;

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

static void
test_pxl_draw_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 20, ry = 20, rw = 10, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_clip_left_no_false_border(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 0, 15, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 0, ry = 5, rw = 15, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_clip_right_no_false_border(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 0, 0, 15, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 5, ry = 5, rw = 15, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_clip_both_sides(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 0, 10, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 0, ry = 5, rw = 20, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_clip_top_no_false_border(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 0, 5, 20, 15);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 5, ry = 0, rw = 10, rh = 15;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_clip_bottom_no_false_border(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 0, 0, 20, 15);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 5, ry = 5, rw = 10, rh = 15;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 3, 4);
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 0, ry = 0, rw = 5, rh = 5;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s = is_inside_scissor(&f.cnv, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}


	fixture_deinit(&f);
}


/* Test Fill Rect ----------------------------------------------------------- */
static inline bool
is_drawn_inside_rect(const pxl_canvas_t *cnv, int x, int y, int rx, int ry, int rw, int rh) {
	assert(rw > 0 && rh > 0);

	rx += cnv->offset_x;
	ry += cnv->offset_y;

	return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static void
test_pxl_fill_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_rect = is_drawn_inside_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_rect_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 20, ry = 20, rw = 10, rh = 10;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_rect = is_drawn_inside_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_rect_fast_path(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&f.cnv, color);

	pxl_canvas_reset_scissor(&f.cnv);

	int rx = 0, ry = 0, rw = w, rh = h;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_rect = is_drawn_inside_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_rect_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 2, 3);
	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&f.cnv, color);

	int rx = 0, ry = 0, rw = 6, rh = 4;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_rect = is_drawn_inside_rect(&f.cnv, x, y, rx, ry, rw, rh);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_rect ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Span -------------------------------------------------------------- */
static inline bool
is_on_span(const pxl_canvas_t *cnv, int x, int y, int span_y, int span_x, int span_w) {
	span_x += cnv->offset_x;
	span_y += cnv->offset_y;
	
	return y == span_y && x >= span_x && x < span_x + span_w;
}

static void
test_pxl_draw_span_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int span_x = 5, span_y = 10, span_w = 10;
	pxl_draw_span(&f.cnv, span_x, span_y, span_w);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool on_span = is_on_span(&f.cnv, x, y, span_y, span_x, span_w);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_span ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_span=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_span, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_span_zero_width(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int span_x = 5, span_y = 10, span_w = 0;
	pxl_draw_span(&f.cnv, span_x, span_y, span_w);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool on_span = is_on_span(&f.cnv, x, y, span_y, span_x, span_w);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_span ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_span=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_span, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_span_clipped(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int span_x = 3, span_y = 7, span_w = 10;
	pxl_draw_span(&f.cnv, span_x, span_y, span_w);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool on_span = is_on_span(&f.cnv, x, y, span_y, span_x, span_w);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_span ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_span=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_span, in_s, want, got);
		}
	}


	fixture_deinit(&f);
}

/* Test Blit -------------------------------------------------------------- */
static inline bool
is_drawn_in_blit(const pxl_canvas_t *cnv, int x, int y,
                 pxl_rect_t pb_r, int cnv_x, int cnv_y) {
	return is_drawn_inside_rect(cnv, x, y, cnv_x, cnv_y, pb_r.w, pb_r.h);
}

static void
test_pxl_blit_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Create source buffer filled with green */
	pxl_buf_t src_pb;
	src_pb.width = 10;
	src_pb.height = 10;
	src_pb.stride = pxl_calc_stride(10);
	src_pb.data = malloc(src_pb.stride * src_pb.height * sizeof(pxl_t));
	if (!st_check(&ST_HERE, src_pb.data != NULL, "src malloc failed")) {
		fixture_deinit(&f);
		return;
	}
	pxl_t color = COLOR_GREEN;
	pxl_buf_fill(&src_pb, color);

	pxl_rect_t pb_r = {0, 0, 10, 10};
	int cnv_x = 5, cnv_y = 5;
	pxl_blit_rect(&f.cnv, &src_pb, pb_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_blit = is_drawn_in_blit(&f.cnv, x, y, pb_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_blit=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_blit, in_s, want, got);
		}
	}

	free(src_pb.data); src_pb.data = NULL;
	fixture_deinit(&f);
}

static void
test_pxl_blit_rect_with_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_buf_t src_pb;
	src_pb.width = 15;
	src_pb.height = 15;
	src_pb.stride = pxl_calc_stride(15);
	src_pb.data = malloc(src_pb.stride * src_pb.height * sizeof(pxl_t));
	if (!st_check(&ST_HERE, src_pb.data != NULL, "src malloc failed")) {
		fixture_deinit(&f);
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_buf_fill(&src_pb, color);

	pxl_rect_t pb_r = {0, 0, 15, 15};
	int cnv_x = 0, cnv_y = 0;
	pxl_blit_rect(&f.cnv, &src_pb, pb_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_blit = is_drawn_in_blit(&f.cnv, x, y, pb_r, cnv_x, cnv_y);

			pxl_t got  = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_blit=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_blit, in_s, want, got);
		}
	}

	free(src_pb.data); src_pb.data = NULL;
	fixture_deinit(&f);
}

static void
test_pxl_blit_rect_fully_clipped(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Create source buffer filled with blue */
	pxl_buf_t src_pb;
	src_pb.width = 10;
	src_pb.height = 10;
	src_pb.stride = pxl_calc_stride(10);
	src_pb.data = malloc(src_pb.stride * src_pb.height * sizeof(pxl_t));
	if (!st_check(&ST_HERE, src_pb.data != NULL, "src malloc failed")) {
		fixture_deinit(&f);
		return;
	}

	pxl_t color = COLOR_BLUE;
	pxl_buf_fill(&src_pb, color);

	pxl_rect_t pb_r = {0, 0, 10, 10};
	int cnv_x = 30, cnv_y = 30;
	pxl_blit_rect(&f.cnv, &src_pb, pb_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_blit = is_drawn_in_blit(&f.cnv, x, y, pb_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_blit=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_blit, in_s, want, got);
		}
	}

	free(src_pb.data); src_pb.data = NULL;
	fixture_deinit(&f);
}

static void
test_pxl_blit_rect_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 5, 5);

	/* Create source buffer filled with yellow */
	pxl_buf_t src_pb;
	src_pb.width = 8;
	src_pb.height = 8;
	src_pb.stride = pxl_calc_stride(8);
	src_pb.data = malloc(src_pb.stride * src_pb.height * sizeof(pxl_t));
	if (!st_check(&ST_HERE, src_pb.data != NULL, "src malloc failed")) {
		fixture_deinit(&f);
		return;
	}

	pxl_t color = COLOR_YELLOW;
	pxl_buf_fill(&src_pb, color);

	pxl_rect_t pb_r = {0, 0, 8, 8};
	int cnv_x = 0, cnv_y = 0;
	pxl_blit_rect(&f.cnv, &src_pb, pb_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_blit = is_drawn_in_blit(&f.cnv, x, y, pb_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_blit=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_blit, in_s, want, got);
		}
	}

	free(src_pb.data); src_pb.data = NULL;
	fixture_deinit(&f);
}

static void
test_pxl_blit_rect_partially_clipped(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Create source buffer filled with white */
	pxl_buf_t src_pb;
	src_pb.width = 15;
	src_pb.height = 15;
	src_pb.stride = pxl_calc_stride(15);
	src_pb.data = malloc(src_pb.stride * src_pb.height * sizeof(pxl_t));
	if (!st_check(&ST_HERE, src_pb.data != NULL, "src malloc failed")) {
		fixture_deinit(&f);
		return;
	}

	pxl_t color = COLOR_YELLOW;
	pxl_buf_fill(&src_pb, color);

	pxl_rect_t pb_r = {0, 0, 15, 15};
	int cnv_x = -5, cnv_y = -5;
	pxl_blit_rect(&f.cnv, &src_pb, pb_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s    = is_inside_scissor(&f.cnv, x, y);
			bool in_blit = is_drawn_in_blit(&f.cnv, x, y, pb_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && in_blit ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_blit=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_blit, in_s, want, got);
		}
	}

	free(src_pb.data); src_pb.data = NULL;
	fixture_deinit(&f);
}

/* Bitmask draw tests ----------------------------------------------------------- */
static inline uint32_t
bitmask_get(const pxl_bitmask_t *bm, int x, int y) {
	assert(bm && bm->data);
	assert(x >= 0 && x < bm->width);
	assert(y >= 0 && y < bm->height);
	assert((bm->stride << 3) >= (size_t)bm->width);

	size_t bit_index = (size_t)y * ((size_t)bm->stride << 3) + (size_t)x;
	size_t byte_index = bit_index >> 3;
	unsigned bit_offset = bit_index & 0x7;

	uint8_t byte = bm->data[byte_index];

	return (uint32_t)((byte >> bit_offset) & 0x1U);
}

static inline bool
is_drawn_on_bitmask(const pxl_canvas_t *cnv, int x, int y,
                    const pxl_bitmask_t *bm, pxl_rect_t bm_r, int cnv_x, int cnv_y) {
	/* Apply offsets */
	cnv_x += cnv->offset_x;
	cnv_y += cnv->offset_y;

	/* Calculate source position in bitmask */
	int bm_x = bm_r.x + x - cnv_x;
	int bm_y = bm_r.y + y - cnv_y;

	/* Check bounds and get bit */
	if (bm_x < bm_r.x || bm_x >= bm_r.x + bm_r.w ||                                                                                   
			bm_y < bm_r.y || bm_y >= bm_r.y + bm_r.h) {                                                                                   
		return false;                                                                                                                 
	} 

	return bitmask_get(bm, bm_x, bm_y) == 1;
}

static void
test_pxl_draw_bitmask_basic(void) {
	int w = 16, h = 16;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Create a bitmask with checkerboard pattern (0xAA = 0b10101010) */
	uint8_t pattern[] = { 0xAA, 0xAA, 0xAA, 0xAA };
	pxl_bitmask_t bm = {
		.data = pattern,
		.width = 8,
		.height = 2,
		.stride = 1
	};

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);
	pxl_rect_t bm_r = {0, 0, 8, 2};
	int cnv_x = 2, cnv_y = 2;
	pxl_draw_bitmask(&f.cnv, &bm, bm_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s       = is_inside_scissor(&f.cnv, x, y);
			bool on_bitmask = is_drawn_on_bitmask(&f.cnv, x, y, &bm, bm_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_bitmask=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_bitmask, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_bitmask_all_bits_set(void) {
	int w = 32, h = 16;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Create a bitmask with all bits set */
	uint8_t pattern[] = { 0xFF, 0xFF };
	pxl_bitmask_t bm = {
		.data = pattern,
		.width = 16,
		.height = 1,
		.stride = 2 
	};

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);
	pxl_rect_t bm_r = {0, 0, 16, 1};
	int cnv_x = 2, cnv_y = 2;
	pxl_draw_bitmask(&f.cnv, &bm, bm_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s       = is_inside_scissor(&f.cnv, x, y);
			bool on_bitmask = is_drawn_on_bitmask(&f.cnv, x, y, &bm, bm_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_bitmask=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_bitmask, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_bitmask_clipped(void) {
	int w = 32, h = 32;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	/* Set scissor to clip the bitmask */
	pxl_canvas_set_scissor(&f.cnv, 4, 4, 8, 8);

	/* Create a bitmask with all bits set, positioned to intersect scissor */
	uint8_t pattern[16];
	for (int i = 0; i < 16; i++) pattern[i] = 0xFF;
	pxl_bitmask_t bm = {
		.data = pattern,
		.width = 16,
		.height = 8,
		.stride = 2 
	};

	pxl_t color = COLOR_BLUE;
	pxl_canvas_set_color(&f.cnv, color);
	pxl_rect_t bm_r = {0, 0, 16, 8};
	int cnv_x = 0, cnv_y = 0;
	pxl_draw_bitmask(&f.cnv, &bm, bm_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s       = is_inside_scissor(&f.cnv, x, y);
			bool on_bitmask = is_drawn_on_bitmask(&f.cnv, x, y, &bm, bm_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_bitmask=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_bitmask, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_bitmask_with_offset(void) {
	int w = 16, h = 16;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) return;

	pxl_canvas_set_offset(&f.cnv, 5, 5);

	/* Create a bitmask with all bits set */
	uint8_t pattern[] = { 0xFF, 0xFF };
	pxl_bitmask_t bm = {
		.data = pattern,
		.width = 8,
		.height = 1,
		.stride = 1
	};

	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&f.cnv, color);
	pxl_rect_t bm_r = {0, 0, 8, 1};
	int cnv_x = 0, cnv_y = 0;
	pxl_draw_bitmask(&f.cnv, &bm, bm_r, cnv_x, cnv_y);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			bool in_s       = is_inside_scissor(&f.cnv, x, y);
			bool on_bitmask = is_drawn_on_bitmask(&f.cnv, x, y, &bm, bm_r, cnv_x, cnv_y);

			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			pxl_t want = in_s && on_bitmask ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_bitmask=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_bitmask, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Span tests */
		ST_T(test_pxl_draw_span_basic),
		ST_T(test_pxl_draw_span_zero_width),
		ST_T(test_pxl_draw_span_clipped),

		/* Line tests */
		ST_T(test_pxl_draw_line_horizontal),
		ST_T(test_pxl_draw_line_vertical),
		ST_T(test_pxl_draw_line_diagonal),
		ST_T(test_pxl_draw_line_outside_scissor),
		ST_T(test_pxl_draw_line_with_offset),
		ST_T(test_pxl_draw_line_with_offset_and_scissor),
		ST_T(test_pxl_draw_line_with_negative_offset),

		/* Rect tests */
		ST_T(test_pxl_draw_rect_basic),
		ST_T(test_pxl_draw_rect_outside_scissor),
		ST_T(test_pxl_draw_rect_clip_left_no_false_border),
		ST_T(test_pxl_draw_rect_clip_right_no_false_border),
		ST_T(test_pxl_draw_rect_clip_both_sides),
		ST_T(test_pxl_draw_rect_clip_top_no_false_border),
		ST_T(test_pxl_draw_rect_clip_bottom_no_false_border),
		ST_T(test_pxl_draw_rect_with_offset),

		/* Fill Rect tests */
		ST_T(test_pxl_fill_rect_basic),
		ST_T(test_pxl_fill_rect_outside_scissor),
		ST_T(test_pxl_fill_rect_fast_path),
		ST_T(test_pxl_fill_rect_with_offset),

		/* Blit tests */
		ST_T(test_pxl_blit_rect_basic),
		ST_T(test_pxl_blit_rect_with_scissor),
		ST_T(test_pxl_blit_rect_fully_clipped),
		ST_T(test_pxl_blit_rect_with_offset),
		ST_T(test_pxl_blit_rect_partially_clipped),

		/* Bitmask tests */
		ST_T(test_pxl_draw_bitmask_basic),
		ST_T(test_pxl_draw_bitmask_all_bits_set),
		ST_T(test_pxl_draw_bitmask_clipped),
		ST_T(test_pxl_draw_bitmask_with_offset)
	);
}
