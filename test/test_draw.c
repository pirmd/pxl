#include <string.h>
#include "canvas.h"
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

static bool
fixture_init(const st_ctx_t *ctx, fixture_t *f, int w, int h) {
	if (!st_check(ctx, pxl_buf_init(&f->pb, w, h) == PXL_SUCCESS, "pixbuf_init failed")) {
		return false;
	}

	if (!st_check(ctx, f->pb.data != NULL, "pixbuf data is NULL")) {
		return false;
	}

	pxl_canvas_init(&f->cnv, &f->pb);
	pxl_buf_zero(&f->pb);
	
	return true;
}

static void
fixture_deinit(fixture_t *f) {
	pxl_buf_deinit(&f->pb);
}

/* Helpers ----------------------------------------------------------------- */
static inline bool
is_inside_scissor(int x, int y, const pxl_canvas_t *cnv) {
	return pxl_in_rect(x, y, cnv->scissor);
}

/* Check if point (x,y) is on the circle outline drawn by draw2d_circle.
 * This helper EXACTLY reproduces the Bresenham algorithm from draw2d_circle
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_circle(int x, int y, int cx, int cy, int r) {
	if (r <= 0) {
		return false;
	}

	int dx = abs(x - cx);
	int dy = abs(y - cy);

	/* Cardinal points (x=0 or y=0) - always drawn */
	if ((dx == 0 && dy == r) || (dx == r && dy == 0)) {
		return true;
	}

	/* Bresenham's circle algorithm - same parameters as draw2d_circle */
	int cx_algo = r;
	int cy_algo = 0;
	int df = 1 - r;
	int d_e = 3;
	int d_se = -2 * r + 5;

	/* Check the 8 symmetric points from the first iteration (r,0) */
	if ((dx == cx_algo && dy == cy_algo) || (dx == cy_algo && dy == cx_algo)) {
		return true;
	}

	cx_algo--;
	while (cy_algo < cx_algo) {
		if (df < 0) {
			df += d_e;
			d_e += 2;
		} else {
			df += d_se;
			d_e += 2;
			d_se += 2;
			cx_algo--;
		}
		cy_algo++;

		/* Check 8 symmetric points */
		if ((dx == cx_algo && dy == cy_algo) || (dx == cy_algo && dy == cx_algo)) {
			return true;
		}
	}

	/* Final iteration when cy_algo == cx_algo */
	if (cy_algo == cx_algo && dx == cx_algo && dy == cy_algo) {
		return true;
	}

	return false;
}

/* Check if point (x,y) is inside the filled circle drawn by draw2d_fill_circle.
 * This helper EXACTLY reproduces the algorithm from draw2d_fill_circle
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_inside_fill_circle(int x, int y, int cx, int cy, int r) {
	if (r <= 0) {
		return false;
	}

	int dx = abs(x - cx);
	int dy = abs(y - cy);

	/* Check center line (y == cy) - drawn first in draw2d_fill_circle */
	if (y == cy && dx <= r) {
		return true;
	}

	/* Check top and bottom single points (x=cx, y=cy±r) */
	if (x == cx && (dy == r)) {
		return true;
	}

	/* Same algorithm as draw2d_fill_circle */
	int c_x = r;
	int c_y = 0;
	int d = 1 - r;

	while (c_x >= c_y) {
		int yy1 = cy + c_y;
		int yy2 = cy - c_y;
		int yy3 = cy + c_x;
		int yy4 = cy - c_x;

		/* Check spans at y = cy ± c_y */
		if ((y == yy1 || y == yy2) && dx <= c_x) {
			return true;
		}

		/* Check spans at y = cy ± c_x (if different from c_y) */
		if (c_x != c_y) {
			if ((y == yy3 || y == yy4) && dx <= c_y) {
				return true;
			}
		}

		c_y++;
		if (d < 0) {
			d += 2 * c_y + 1;
		} else {
			c_x--;
			d += 2 * (c_y - c_x) + 1;
		}
	}

	return false;
}

/* Check if point (x,y) is on a horizontal span at span_y from span_x to span_x+span_w */
static inline bool
is_on_span(int x, int y, int span_y, int span_x, int span_w) {
	return y == span_y && x >= span_x && x < span_x + span_w;
}

/* Forward declarations */
static inline bool is_drawn_on_line(int x, int y, int x0, int y0, int x1, int y1, pxl_rect_t scissor);

/* Check if point (x,y) is on the triangle outline (one of the 3 edges).
 * Uses the same line drawing logic as draw2d_line via is_drawn_on_line. */
static inline bool
is_on_triangle(int x, int y, int x0, int y0, int x1, int y1, int x2, int y2, pxl_rect_t scissor) {
	return is_drawn_on_line(x, y, x0, y0, x1, y1, scissor) ||
	       is_drawn_on_line(x, y, x1, y1, x2, y2, scissor) ||
	       is_drawn_on_line(x, y, x2, y2, x0, y0, scissor);
}

/* Check if point (x,y) is inside triangle using edge function method */
static inline bool
is_in_triangle(int x, int y, int x0, int y0, int x1, int y1, int x2, int y2) {
	/* Edge function: (x - ax) * (by - ay) - (y - ay) * (bx - ax) */
	/* For edge 0-1 */
	int d1 = (x - x0) * (y1 - y0) - (y - y0) * (x1 - x0);
	/* For edge 1-2 */
	int d2 = (x - x1) * (y2 - y1) - (y - y1) * (x2 - x1);
	/* For edge 2-0 */
	int d3 = (x - x2) * (y0 - y2) - (y - y2) * (x0 - x2);

	/* All edge functions must have the same sign (or zero) for point to be inside */
	return (d1 >= 0 && d2 >= 0 && d3 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0);
}

/* Check if point (x,y) would be drawn by draw2d_rect with given params and scissor.
 * This helper EXACTLY reproduces the logic from draw2d_rect
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_rect(int x, int y, int rx, int ry, int rw, int rh, pxl_rect_t scissor) {
	if (rw <= 0 || rh <= 0) {
		return false;
	}

	/* Simulate pxl_clip_rect */
	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){rx, ry, rw, rh}, scissor, &r)) {
		return false;
	}

	/* Check if point is on top border (original top edge matches) */
	if (r.y == ry && y == r.y && x >= r.x && x < r.x + r.w) {
		return true;
	}

	/* Check if point is on bottom border (original bottom edge matches) */
	if (r.y + r.h == ry + rh) {
		int bottom_y = r.y + r.h;
		if (y == bottom_y - 1 && x >= r.x && x < r.x + r.w) {
			return true;
		}
	}

	/* Check if point is on left border (original left edge matches) */
	if (r.x == rx && x == r.x && y >= r.y && y < r.y + r.h) {
		return true;
	}

	/* Check if point is on right border (original right edge matches) */
	if (r.x + r.w == rx + rw) {
		int right_x = r.x + r.w;
		if (x == right_x - 1 && y >= r.y && y < r.y + r.h) {
			return true;
		}
	}

	return false;
}

/* Check if point (x,y) would be filled by draw2d_fill_rect with given params and scissor.
 * This helper EXACTLY reproduces the logic from draw2d_fill_rect
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_inside_fill_rect(int x, int y, int rx, int ry, int rw, int rh, pxl_rect_t scissor) {
	if (rw <= 0 || rh <= 0) {
		return false;
	}

	/* Simulate pxl_clip_rect */
	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){rx, ry, rw, rh}, scissor, &r)) {
		return false;
	}

	return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

/* Check if point (x,y) would be drawn by draw2d_line with given params and scissor.
 * This helper EXACTLY reproduces the logic from draw2d_line
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_line(int x, int y, int x0, int y0, int x1, int y1, pxl_rect_t scissor) {
	/* Quick reject with bounding box */
	int min_x = pxl_min(x0, x1);
	int min_y = pxl_min(y0, y1);
	int w = abs(x1 - x0) + 1;
	int h = abs(y1 - y0) + 1;
	
	/* Simulate canvas_quick_reject */
	if (min_x >= scissor.x + scissor.w || min_x + w <= scissor.x ||
	    min_y >= scissor.y + scissor.h || min_y + h <= scissor.y) {
		return false;
	}

	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {  /* X-major line */
		int err = dx / 2;
		for (;;) {
			/* Check if this point would be drawn by draw_span */
			if (y0 >= scissor.y && y0 < scissor.y + scissor.h) {
				pxl_span_t span;
				if (pxl_clip_span((pxl_span_t){x0, 1}, (pxl_span_t){scissor.x, scissor.w}, &span)) {
					if (x == span.x && y == y0) {
						return true;
					}
				}
			}
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
			/* Check if this point would be drawn by draw_span */
			if (y0 >= scissor.y && y0 < scissor.y + scissor.h) {
				pxl_span_t span;
				if (pxl_clip_span((pxl_span_t){x0, 1}, (pxl_span_t){scissor.x, scissor.w}, &span)) {
					if (x == span.x && y == y0) {
						return true;
					}
				}
			}
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

/* Test Line -------------------------------------------------------------- */
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

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw line completely outside scissor */
	int x0 = 20, y0 = 20, x1 = 30, y1 = 30;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(x, y, x0, y0, x1, y1, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_line=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_line, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Rect -------------------------------------------------------------- */

static void
test_pxl_draw_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw a 10x10 rect at (5,5) */
	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_rect_zero_size(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect with zero size - should do nothing */
	/* Note: only the last call matters for the test since all draw to the same pixbuf */
	int rx = 5, ry = 5, rw = 0, rh = 10;
	pxl_draw_rect(&f.cnv, 5, 5, 0, 0);
	pxl_draw_rect(&f.cnv, 5, 5, 10, 0);
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Draw rect completely outside scissor */
	int rx = 20, ry = 20, rw = 10, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set scissor starting at x=5 */
	pxl_canvas_set_scissor(&f.cnv, 5, 0, 15, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect from x=0 (outside scissor left) to x=14 */
	/* Clipped rect: x=5, w=10. Original left edge at x=0 is outside, so no left border at x=5 */
	int rx = 0, ry = 5, rw = 15, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set scissor ending at x=14 */
	pxl_canvas_set_scissor(&f.cnv, 0, 0, 15, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect from x=5 to x=20 (outside scissor right) */
	/* Clipped rect: x=5, w=10. Original right edge at x=20 is outside, so no right border at x=14 */
	int rx = 5, ry = 5, rw = 15, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set scissor in the middle */
	pxl_canvas_set_scissor(&f.cnv, 5, 0, 10, 20);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect from x=0 to x=20, clipped to x=5, w=10 */
	/* Both original edges are outside, so no vertical borders at all */
	int rx = 0, ry = 5, rw = 20, rh = 10;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set scissor starting at y=5 */
	pxl_canvas_set_scissor(&f.cnv, 0, 5, 20, 15);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect from y=0 (outside scissor top) to y=14 */
	/* Clipped rect: y=5, h=10. Original top edge at y=0 is outside, so no top border at y=5 */
	int rx = 5, ry = 0, rw = 10, rh = 15;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Set scissor ending at y=14 */
	pxl_canvas_set_scissor(&f.cnv, 0, 0, 20, 15);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect from y=5 to y=20 (outside scissor bottom) */
	/* Clipped rect: y=5, h=10. Original bottom edge at y=20 is outside, so no bottom border at y=14 */
	int rx = 5, ry = 5, rw = 10, rh = 15;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Fill Rect ----------------------------------------------------------- */

static void
test_pxl_fill_rect_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw a filled 10x10 rect at (5,5) */
	int rx = 5, ry = 5, rw = 10, rh = 10;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool pxl_in_rect = is_drawn_inside_fill_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = pxl_in_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): pxl_in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_rect_zero_size(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect with zero size - should do nothing */
	/* Note: only the last call matters for the test since all draw to the same pixbuf */
	int rx = 5, ry = 5, rw = 0, rh = 10;
	pxl_fill_rect(&f.cnv, 5, 5, 0, 0);
	pxl_fill_rect(&f.cnv, 5, 5, 10, 0);
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool pxl_in_rect = is_drawn_inside_fill_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = pxl_in_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): pxl_in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
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

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw rect completely outside scissor */
	int rx = 20, ry = 20, rw = 10, rh = 10;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool pxl_in_rect = is_drawn_inside_fill_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = pxl_in_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): pxl_in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
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

	/* Set scissor to full canvas */
	pxl_canvas_reset_scissor(&f.cnv);

	/* Draw rect covering entire scissor - should use fast path */
	int rx = 0, ry = 0, rw = w, rh = h;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool pxl_in_rect = is_drawn_inside_fill_rect(x, y, rx, ry, rw, rh, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = pxl_in_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): pxl_in_rect=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, pxl_in_rect, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Circle ------------------------------------------------------------- */

static void
test_pxl_draw_circle_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	int cx = 10, cy = 10, r = 5;
	pxl_draw_circle(&f.cnv, cx, cy, r);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_circle = is_drawn_on_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_circle && in_s;

			pxl_t want = should_be_colored ? color: 0x00;
			ST_CHECK((got == want),
			         "pixel (%d,%d): on_circle=%d, inside_scissor=%d, got=0x%08X, want=0x%08X",
			         x, y, on_circle, in_s, got, want);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_circle_zero_radius(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw circle with zero/negative radius - should do nothing */
	/* Note: only the last call matters for the test since all draw to the same pixbuf */
	int cx = 10, cy = 10, r = -1;
	pxl_draw_circle(&f.cnv, cx, cy, 0);
	pxl_draw_circle(&f.cnv, cx, cy, r);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_circle = is_drawn_on_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_circle, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_circle_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw circle completely outside scissor */
	int cx = 20, cy = 20, r = 5;
	pxl_draw_circle(&f.cnv, cx, cy, r);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_circle = is_drawn_on_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_circle, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Fill Circle --------------------------------------------------------- */

static void
test_pxl_fill_circle_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled circle at (10,10) with radius 5 */
	int cx = 10, cy = 10, r = 5;
	pxl_fill_circle(&f.cnv, cx, cy, r);

	int filled_count = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_circle = is_drawn_inside_fill_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			if (should_be_colored) {
				filled_count++;
			}

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_circle, in_s, want, got);
		}
	}

	/* Verify we actually filled some pixels */
	ST_CHECK(filled_count > 0, "expected at least one pixel filled, got %d", filled_count);

	fixture_deinit(&f);
}

static void
test_pxl_fill_circle_zero_radius(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled circle with zero/negative radius - should do nothing */
	/* Note: only the last call matters for the test since all draw to the same pixbuf */
	int cx = 10, cy = 10, r = -1;
	pxl_fill_circle(&f.cnv, cx, cy, 0);
	pxl_fill_circle(&f.cnv, cx, cy, r);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_circle = is_drawn_inside_fill_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_circle, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_circle_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled circle completely outside scissor */
	int cx = 20, cy = 20, r = 5;
	pxl_fill_circle(&f.cnv, cx, cy, r);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_circle = is_drawn_inside_fill_circle(x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_circle, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Triangle ---------------------------------------------------------- */

static void
test_pxl_draw_triangle_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw triangle with vertices at (5,5), (15,5), (10,15) */
	int x0 = 5, y0 = 5, x1 = 15, y1 = 5, x2 = 10, y2 = 15;
	pxl_draw_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_tri = is_on_triangle(x, y, x0, y0, x1, y1, x2, y2, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_tri && in_s;

			pxl_t want = should_be_colored ? color : 0x00;
			ST_CHECK(got == want,
			         "pixel (%d,%d): on_triangle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_tri, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_triangle_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw triangle completely outside scissor */
	int x0 = 20, y0 = 20, x1 = 30, y1 = 20, x2 = 25, y2 = 30;
	pxl_draw_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_tri = is_on_triangle(x, y, x0, y0, x1, y1, x2, y2, f.cnv.scissor);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_tri && in_s;

			pxl_t want = should_be_colored ? color : 0x00;
			ST_CHECK(got == want,
			         "pixel (%d,%d): on_triangle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_tri, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Fill Triangle ------------------------------------------------------ */

static void
test_pxl_fill_triangle_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled triangle with vertices at (5,5), (15,5), (10,15) */
	int x0 = 5, y0 = 5, x1 = 15, y1 = 5, x2 = 10, y2 = 15;
	pxl_fill_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	int filled_count = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_tri = is_in_triangle(x, y, x0, y0, x1, y1, x2, y2);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_tri && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			if (should_be_colored) {
				filled_count++;
			}

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_triangle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_tri, in_s, want, got);
		}
	}

	/* Verify we actually filled some pixels */
	ST_CHECK(filled_count > 0, "expected at least one pixel filled, got %d", filled_count);

	fixture_deinit(&f);
}

static void
test_pxl_fill_triangle_outside_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled triangle completely outside scissor */
	int x0 = 20, y0 = 20, x1 = 30, y1 = 20, x2 = 25, y2 = 30;
	pxl_fill_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_tri = is_in_triangle(x, y, x0, y0, x1, y1, x2, y2);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_tri && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): in_triangle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, in_tri, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Line tests */
		ST_T(test_pxl_draw_line_horizontal),
		ST_T(test_pxl_draw_line_vertical),
		ST_T(test_pxl_draw_line_diagonal),
		ST_T(test_pxl_draw_line_outside_scissor),

		/* Rect tests */
		ST_T(test_pxl_draw_rect_basic),
		ST_T(test_pxl_draw_rect_zero_size),
		ST_T(test_pxl_draw_rect_outside_scissor),
		ST_T(test_pxl_draw_rect_clip_left_no_false_border),
		ST_T(test_pxl_draw_rect_clip_right_no_false_border),
		ST_T(test_pxl_draw_rect_clip_both_sides),
		ST_T(test_pxl_draw_rect_clip_top_no_false_border),
		ST_T(test_pxl_draw_rect_clip_bottom_no_false_border),

		/* Fill Rect tests */
		ST_T(test_pxl_fill_rect_basic),
		ST_T(test_pxl_fill_rect_zero_size),
		ST_T(test_pxl_fill_rect_outside_scissor),
		ST_T(test_pxl_fill_rect_fast_path),

		/* Circle tests */
		ST_T(test_pxl_draw_circle_basic),
		ST_T(test_pxl_draw_circle_zero_radius),
		ST_T(test_pxl_draw_circle_outside_scissor),

		/* Fill Circle tests */
		ST_T(test_pxl_fill_circle_basic),
		ST_T(test_pxl_fill_circle_zero_radius),
		ST_T(test_pxl_fill_circle_outside_scissor),

		/* Triangle tests */
		ST_T(test_pxl_draw_triangle_basic),
		ST_T(test_pxl_draw_triangle_outside_scissor),

		/* Fill Triangle tests */
		ST_T(test_pxl_fill_triangle_basic),
		ST_T(test_pxl_fill_triangle_outside_scissor)

	);
}
