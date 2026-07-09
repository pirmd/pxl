#include <string.h>
#include "canvas.h"
#include "draw.h"
#include "draw_extra.h"
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

/* Check if point (x,y) is on the circle outline drawn by pxl_draw_circle.
 * This helper EXACTLY reproduces the Bresenham algorithm from pxl_draw_circle
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_circle(const pxl_canvas_t *cnv, int x, int y, int cx, int cy, int r) {
	cx += cnv->offset_x;
	cy += cnv->offset_y;
	
	if (r <= 0) {
		return false;
	}

	int dx = abs(x - cx);
	int dy = abs(y - cy);

	/* Cardinal points (x=0 or y=0) - always drawn */
	if ((dx == 0 && dy == r) || (dx == r && dy == 0)) {
		return true;
	}

	/* Bresenham's circle algorithm - same parameters as pxl_draw_circle */
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

/* Check if point (x,y) is inside the filled circle drawn by pxl_fill_circle.
 * This helper EXACTLY reproduces the algorithm from pxl_fill_circle
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_inside_fill_circle(const pxl_canvas_t *cnv, int x, int y, int cx, int cy, int r) {
	cx += cnv->offset_x;
	cy += cnv->offset_y;
	
	if (r <= 0) {
		return false;
	}

	int dx = abs(x - cx);
	int dy = abs(y - cy);

	/* Check center line (y == cy) - drawn first in pxl_fill_circle */
	if (y == cy && dx <= r) {
		return true;
	}

	/* Check top and bottom single points (x=cx, y=cy±r) */
	if (x == cx && (dy == r)) {
		return true;
	}

	/* Same algorithm as pxl_fill_circle */
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

/* Check if point (x,y) is inside triangle using edge function method */
static inline bool
is_in_triangle(const pxl_canvas_t *cnv, int x, int y, int x0, int y0, int x1, int y1, int x2, int y2) {
	x0 += cnv->offset_x;
	y0 += cnv->offset_y;
	x1 += cnv->offset_x;
	y1 += cnv->offset_y;
	x2 += cnv->offset_x;
	y2 += cnv->offset_y;
	
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

/* Check if point (x,y) would be drawn by pxl_draw_line with given params.
 * This helper EXACTLY reproduces the logic from pxl_draw_line
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_line(const pxl_canvas_t *cnv, int x, int y, int x0, int y0, int x1, int y1) {
	x0 += cnv->offset_x;
	y0 += cnv->offset_y;
	x1 += cnv->offset_x;
	y1 += cnv->offset_y;
	
	const pxl_rect_t *scissor = &cnv->scissor;
	
	/* Quick reject with bounding box */
	int min_x = pxl_min(x0, x1);
	int min_y = pxl_min(y0, y1);
	int w = abs(x1 - x0) + 1;
	int h = abs(y1 - y0) + 1;
	
	/* Simulate canvas_quick_reject */
	if (min_x >= scissor->x + scissor->w || min_x + w <= scissor->x ||
	    min_y >= scissor->y + scissor->h || min_y + h <= scissor->y) {
		return false;
	}

	int dx = abs(x1 - x0), sx = (x0 < x1) ? 1 : -1;
	int dy = abs(y1 - y0), sy = (y0 < y1) ? 1 : -1;

	if (dx >= dy) {
		int err = dx / 2;
		for (;;) {
			if (y0 >= scissor->y && y0 < scissor->y + scissor->h) {
				pxl_span_t span;
				if (pxl_clip_span((pxl_span_t){x0, 1}, (pxl_span_t){scissor->x, scissor->w}, &span)) {
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
	} else {
		int err = dy / 2;
		for (;;) {
			if (y0 >= scissor->y && y0 < scissor->y + scissor->h) {
				pxl_span_t span;
				if (pxl_clip_span((pxl_span_t){x0, 1}, (pxl_span_t){scissor->x, scissor->w}, &span)) {
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

/* Check if point (x,y) is on the triangle outline (one of the 3 edges).
 * Uses the same line drawing logic as pxl_draw_line via is_drawn_on_line. */
static inline bool
is_on_triangle(const pxl_canvas_t *cnv, int x, int y, int x0, int y0, int x1, int y1, int x2, int y2) {
	return is_drawn_on_line(cnv, x, y, x0, y0, x1, y1) ||
	       is_drawn_on_line(cnv, x, y, x1, y1, x2, y2) ||
	       is_drawn_on_line(cnv, x, y, x2, y2, x0, y0);
}

/* Test Circle ----------------------------------------------------------- */

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
			bool on_circle = is_drawn_on_circle(&f.cnv, x, y, cx, cy, r);
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
			bool on_circle = is_drawn_on_circle(&f.cnv, x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_circle=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_circle, in_s, got, want);
		}
	}

	fixture_deinit(&f);
}

/* Test Fill Circle ------------------------------------------------------- */

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
			bool in_circle = is_drawn_inside_fill_circle(&f.cnv, x, y, cx, cy, r);
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
			bool in_circle = is_drawn_inside_fill_circle(&f.cnv, x, y, cx, cy, r);
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

/* Test Triangle --------------------------------------------------------- */

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
			bool on_tri = is_on_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
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
			bool on_tri = is_on_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
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
			bool in_tri = is_in_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
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
			bool in_tri = is_in_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
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

/* Test Offset ------------------------------------------------------------- */

static void
test_pxl_draw_circle_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 5, 5);
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw circle at (0,0) with radius 4 - with offset this becomes (5,5) with r=4 */
	int cx = 0, cy = 0, r = 4;
	pxl_draw_circle(&f.cnv, cx, cy, r);

	int expected_count = 0;
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_circle = is_drawn_on_circle(&f.cnv, x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			if (on_circle && in_s) expected_count++;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}
	
	ST_CHECK(expected_count > 0, "expected circle to have some pixels drawn");

	fixture_deinit(&f);
}

static void
test_pxl_fill_circle_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 3, 3);
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled circle at (0,0) with radius 5 - with offset this becomes (3,3) with r=5 */
	int cx = 0, cy = 0, r = 5;
	pxl_fill_circle(&f.cnv, cx, cy, r);

	int filled_count = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_circle = is_drawn_inside_fill_circle(&f.cnv, x, y, cx, cy, r);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_circle && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			if (in_circle && in_s) filled_count++;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}
	
	ST_CHECK(filled_count > 0, "expected filled circle to have some pixels");

	fixture_deinit(&f);
}

static void
test_pxl_draw_triangle_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 2, 2);
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw triangle with vertices at (0,0), (10,0), (5,10) */
	int x0 = 0, y0 = 0, x1 = 10, y1 = 0, x2 = 5, y2 = 10;
	pxl_draw_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_tri = is_on_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_tri && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_fill_triangle_with_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 1, 1);
	pxl_t color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw filled triangle with vertices at (0,0), (8,0), (4,8) */
	int x0 = 0, y0 = 0, x1 = 8, y1 = 0, x2 = 4, y2 = 8;
	pxl_fill_triangle(&f.cnv, x0, y0, x1, y1, x2, y2);

	int filled_count = 0;
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_tri = is_in_triangle(&f.cnv, x, y, x0, y0, x1, y1, x2, y2);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_tri && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			if (in_tri && in_s) {
				filled_count++;
			}
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}
	
	ST_CHECK(filled_count > 0, "expected filled triangle to have some pixels, got %d", filled_count);

	fixture_deinit(&f);
}

/* Main ----------------------------------------------------------------- */
int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Circle tests */
		ST_T(test_pxl_draw_circle_basic),
		ST_T(test_pxl_draw_circle_outside_scissor),

		/* Fill Circle tests */
		ST_T(test_pxl_fill_circle_basic),
		ST_T(test_pxl_fill_circle_outside_scissor),

		/* Triangle tests */
		ST_T(test_pxl_draw_triangle_basic),
		ST_T(test_pxl_draw_triangle_outside_scissor),

		/* Fill Triangle tests */
		ST_T(test_pxl_fill_triangle_basic),
		ST_T(test_pxl_fill_triangle_outside_scissor),

		/* Offset tests */
		ST_T(test_pxl_draw_circle_with_offset),
		ST_T(test_pxl_fill_circle_with_offset),
		ST_T(test_pxl_draw_triangle_with_offset),
		ST_T(test_pxl_fill_triangle_with_offset)
	);
}
