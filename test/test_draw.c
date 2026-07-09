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

/* Check if point (x,y) would be drawn by pxl_draw_line with given params.
 * This helper EXACTLY reproduces the logic from pxl_draw_line
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_line(const pxl_canvas_t *cnv, int x, int y, int x0, int y0, int x1, int y1) {
	/* Apply offsets */
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

	if (dx >= dy) {  /* X-major line */
		int err = dx / 2;
		for (;;) {
			/* Check if this point would be drawn by pxl_draw_span */
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
	} else {  /* Y-major line */
		int err = dy / 2;
		for (;;) {
			/* Check if this point would be drawn by pxl_draw_span */
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

/* Check if point (x,y) would be drawn by pxl_draw_rect with given params.
 * This helper EXACTLY reproduces the logic from pxl_draw_rect
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_on_rect(const pxl_canvas_t *cnv, int x, int y, int rx, int ry, int rw, int rh) {
	rx += cnv->offset_x;
	ry += cnv->offset_y;
	
	const pxl_rect_t *scissor = &cnv->scissor;
	
	if (rw <= 0 || rh <= 0) {
		return false;
	}

	/* Simulate pxl_clip_rect */
	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){rx, ry, rw, rh}, *scissor, &r)) {
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

/* Check if point (x,y) would be filled by pxl_fill_rect with given params.
 * This helper EXACTLY reproduces the logic from pxl_fill_rect
 * to ensure 100% consistency between drawing and testing. */
static inline bool
is_drawn_inside_fill_rect(const pxl_canvas_t *cnv, int x, int y, int rx, int ry, int rw, int rh) {
	rx += cnv->offset_x;
	ry += cnv->offset_y;
	
	const pxl_rect_t *scissor = &cnv->scissor;
	
	if (rw <= 0 || rh <= 0) {
		return false;
	}

	/* Simulate pxl_clip_rect */
	pxl_rect_t r;
	if (!pxl_clip_rect((pxl_rect_t){rx, ry, rw, rh}, *scissor, &r)) {
		return false;
	}

	return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}

/* Check if point (x,y) is on a horizontal span at span_y from span_x to span_x+span_w */
static inline bool
is_on_span(const pxl_canvas_t *cnv, int x, int y, int span_y, int span_x, int span_w) {
	span_x += cnv->offset_x;
	span_y += cnv->offset_y;
	
	return y == span_y && x >= span_x && x < span_x + span_w;
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
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
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
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
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
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
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
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool pxl_in_rect = is_drawn_inside_fill_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool pxl_in_rect = is_drawn_inside_fill_rect(&f.cnv, x, y, rx, ry, rw, rh);
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
			bool pxl_in_rect = is_drawn_inside_fill_rect(&f.cnv, x, y, rx, ry, rw, rh);
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

/* Test Span -------------------------------------------------------------- */

static void
test_pxl_draw_span_basic(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw a span at y=10, x=5 to 14 */
	int span_x = 5, span_y = 10, span_w = 10;
	pxl_draw_span(&f.cnv, span_x, span_y, span_w);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_span = is_on_span(&f.cnv, x, y, span_y, span_x, span_w);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_span && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

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

	/* Draw span with zero width - should do nothing */
	pxl_draw_span(&f.cnv, 5, 10, 0);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			ST_CHECK(got == 0x00,
			         "pixel (%d,%d): expected 0x00, got=0x%08X",
			         x, y, got);
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

	/* Set a small scissor */
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);

	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw span partially outside scissor */
	pxl_draw_span(&f.cnv, 3, 7, 10);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_span = is_on_span(&f.cnv, x, y, 7, 3, 10);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_span && in_s;
			pxl_t want = should_be_colored ? color : 0x00;

			ST_CHECK(got == want,
			         "pixel (%d,%d): on_span=%d, inside_scissor=%d, want=0x%08X, got=0x%08X",
			         x, y, on_span, in_s, want, got);
		}
	}

	fixture_deinit(&f);
}

/* Test Offset ------------------------------------------------------------- */

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

	/* Draw line at (0,0) to (9,0) - with offset this becomes (5,5) to (14,5) */
	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
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

	/* Draw rect at (0,0) with w=5, h=5 - with offset this becomes (3,4) to (7,8) */
	int rx = 0, ry = 0, rw = 5, rh = 5;
	pxl_draw_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_rect = is_drawn_on_rect(&f.cnv, x, y, rx, ry, rw, rh);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
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

	/* Draw filled rect at (0,0) with w=6, h=4 - with offset this becomes (2,3) to (7,6) */
	int rx = 0, ry = 0, rw = 6, rh = 4;
	pxl_fill_rect(&f.cnv, rx, ry, rw, rh);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool in_rect = is_drawn_inside_fill_rect(&f.cnv, x, y, rx, ry, rw, rh);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = in_rect && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_with_offset_and_scissor(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set both offset and scissor */
	pxl_canvas_set_offset(&f.cnv, 10, 10);
	pxl_canvas_set_scissor(&f.cnv, 5, 5, 10, 10);
	
	pxl_t color = COLOR_YELLOW;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw line at (0,0) to (5,0) - with offset becomes (10,10) to (15,10) */
	/* But scissor is (5,5) to (14,14), so only (10,10) to (14,10) should be visible */
	int x0 = 0, y0 = 0, x1 = 5, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_pxl_draw_with_negative_offset(void) {
	int w = 20, h = 20;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	/* Set negative offset - draws outside buffer, should be clipped */
	pxl_canvas_set_offset(&f.cnv, -5, -5);
	
	pxl_t color = COLOR_RED;
	pxl_canvas_set_color(&f.cnv, color);

	/* Draw line at (0,0) to (9,0) - with offset becomes (-5,-5) to (4,-5) */
	/* All should be clipped (y=-5 is outside buffer) */
	int x0 = 0, y0 = 0, x1 = 9, y1 = 0;
	pxl_draw_line(&f.cnv, x0, y0, x1, y1);

	/* Buffer should remain empty */
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			bool on_line = is_drawn_on_line(&f.cnv, x, y, x0, y0, x1, y1);
			bool in_s = is_inside_scissor(x, y, &f.cnv);
			bool should_be_colored = on_line && in_s;
			pxl_t want = should_be_colored ? color : 0x00;
			
			ST_CHECK(got == want,
			         "pixel (%d,%d): want 0x%08X, got 0x%08X",
			         x, y, want, got);
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

		/* Rect tests */
		ST_T(test_pxl_draw_rect_basic),
		ST_T(test_pxl_draw_rect_outside_scissor),
		ST_T(test_pxl_draw_rect_clip_left_no_false_border),
		ST_T(test_pxl_draw_rect_clip_right_no_false_border),
		ST_T(test_pxl_draw_rect_clip_both_sides),
		ST_T(test_pxl_draw_rect_clip_top_no_false_border),
		ST_T(test_pxl_draw_rect_clip_bottom_no_false_border),

		/* Fill Rect tests */
		ST_T(test_pxl_fill_rect_basic),
		ST_T(test_pxl_fill_rect_outside_scissor),
		ST_T(test_pxl_fill_rect_fast_path),

		/* Offset tests */
		ST_T(test_pxl_draw_line_with_offset),
		ST_T(test_pxl_draw_rect_with_offset),
		ST_T(test_pxl_fill_rect_with_offset),
		ST_T(test_pxl_draw_with_offset_and_scissor),
		ST_T(test_pxl_draw_with_negative_offset)
	);
}
