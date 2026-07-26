#include <string.h>
#include "canvas.h"
#include "buf.h"
#include "stest/stest.h"

/* Fixture ----------------------------------------------------------------- */

#define COLOR_WHITE  0xFFFFFFFFU  /* Opaque white */
#define COLOR_RED    0xFFFF0000U  /* Opaque red */
#define COLOR_GREEN  0xFF00FF00U  /* Opaque green */
#define COLOR_BLUE   0xFF0000FFU  /* Opaque blue */
#define COLOR_TRANS  0x00000000U  /* Fully transparent */

static void
pxl_buf_zero(pxl_buf_t *pb) {
	assert(pb && pb->data);
	memset(pb->data, 0x00, pb->height * pb->stride * sizeof(pxl_t));
}

static void
pxl_buf_clear(pxl_buf_t *pb, pxl_t color) {
	assert(pb);
	
	pxl_t *dst = pb->data;
	for (int y = 0; y < pb->height; ++y) {
		for (int x = 0; x < pb->width; ++x) {
			dst[x] = color;
		}
		dst += pb->stride;
	}
}

typedef struct {
	pxl_buf_t pb;
	pxl_canvas_t cnv;
} fixture_t;

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
in_scissor(int x, int y, const pxl_canvas_t *cnv) {
	const pxl_rect_t *s = &cnv->scissor;
	return x >= s->x && x < s->x + s->w && y >= s->y && y < s->y + s->h;
}

/* Test Lifecycle ---------------------------------------------------------- */

static void
test_canvas_init_basic(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	ST_CHECK(f.cnv.pb == &f.pb, "canvas should reference the pixbuf");
	ST_CHECK(f.cnv.color == COLOR_WHITE, "default color should be white");
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == w && f.cnv.scissor.h == h,
	         "scissor size: want %dx%d, got %dx%d", w, h,
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_set_color(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_t want_color = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, want_color);
	ST_CHECK(f.cnv.color == want_color,
	         "color: want 0x%08X, got 0x%08X",
	         want_color, f.cnv.color);

	fixture_deinit(&f);
}

static void
test_canvas_set_scissor(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 10, 20, 30, 40);
	ST_CHECK(f.cnv.scissor.x == 10 && f.cnv.scissor.y == 20,
	         "scissor origin: want (10,20), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 30 && f.cnv.scissor.h == 40,
	         "scissor size: want 30x40, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_set_scissor_clipped(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, -10, -10, 200, 200);
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin clamped: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 100 && f.cnv.scissor.h == 100,
	         "scissor size clamped: want 100x100, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_reset_scissor(void) {
	int w = 50, h = 60;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 10, 10, 20, 20);
	ST_CHECK(f.cnv.scissor.x == 10,
	         "scissor modified: want x=10, got x=%d",
	         f.cnv.scissor.x);

	pxl_canvas_reset_scissor(&f.cnv);
	ST_CHECK(f.cnv.scissor.x == 0 && f.cnv.scissor.y == 0,
	         "scissor origin reset: want (0,0), got (%d,%d)",
	         f.cnv.scissor.x, f.cnv.scissor.y);
	ST_CHECK(f.cnv.scissor.w == 50 && f.cnv.scissor.h == 60,
	         "scissor size reset: want 50x60, got %dx%d",
	         f.cnv.scissor.w, f.cnv.scissor.h);

	fixture_deinit(&f);
}

static void
test_canvas_offset_init_zero(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	ST_CHECK(f.cnv.offset_x == 0 && f.cnv.offset_y == 0,
	         "offsets should be zero by default: got (%d,%d)",
	         f.cnv.offset_x, f.cnv.offset_y);

	fixture_deinit(&f);
}

static void
test_canvas_set_offset(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 10, 20);
	ST_CHECK(f.cnv.offset_x == 10 && f.cnv.offset_y == 20,
	         "offset: want (10,20), got (%d,%d)",
	         f.cnv.offset_x, f.cnv.offset_y);

	pxl_canvas_set_offset(&f.cnv, -5, -3);
	ST_CHECK(f.cnv.offset_x == -5 && f.cnv.offset_y == -3,
	         "offset: want (-5,-3), got (%d,%d)",
	         f.cnv.offset_x, f.cnv.offset_y);

	fixture_deinit(&f);
}

static void
test_canvas_reset_offset(void) {
	int w = 100, h = 100;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_offset(&f.cnv, 15, 25);
	ST_CHECK(f.cnv.offset_x == 15 && f.cnv.offset_y == 25,
	         "offset set: want (15,25), got (%d,%d)",
	         f.cnv.offset_x, f.cnv.offset_y);

	pxl_canvas_reset_offset(&f.cnv);
	ST_CHECK(f.cnv.offset_x == 0 && f.cnv.offset_y == 0,
	         "offset reset: want (0,0), got (%d,%d)",
	         f.cnv.offset_x, f.cnv.offset_y);

	fixture_deinit(&f);
}


/* Test clear -------------------------------------------------------------- */

static void
test_canvas_clear_white(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_canvas_set_scissor(&f.cnv, 2, 2, 6, 6);
	
	pxl_t fg = COLOR_GREEN;
	pxl_canvas_set_color(&f.cnv, fg);
	
	pxl_canvas_clear(&f.cnv);

	for (int y = 0; y < w; ++y) {
		for (int x = 0; x < h; ++x) {
			pxl_t  got = *pxl_buf_ptr(&f.pb, x, y);
			bool inside_scissor = in_scissor(x, y, &f.cnv);
			pxl_t want =  inside_scissor ? fg: 0x00;
			ST_CHECK(got == want,
			         "pixel (%d,%d)%s: want 0x%08X, got 0x%08X",
			         x, y, inside_scissor ? " in scissor": "", want, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_clear_full_black(void) {
	int w = 4, h = 4;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_buf_clear(&f.pb, COLOR_TRANS);  /* Fill with non-zero then clear with 0 */
	pxl_canvas_set_color(&f.cnv, COLOR_TRANS);
	pxl_canvas_clear(&f.cnv);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			ST_CHECK(got == COLOR_TRANS, "pixel (%d,%d) should be transparent (fast path), got 0x%08X", x, y, got);
		}
	}

	fixture_deinit(&f);
}

static void
test_canvas_clear_full_white(void) {
	int w = 10, h = 10;
	fixture_t f;
	if (!fixture_init(&ST_HERE, &f, w, h)) {
		return;
	}

	pxl_buf_clear(&f.pb, COLOR_RED);  /* Fill with non-white pattern */
	pxl_canvas_set_color(&f.cnv, COLOR_WHITE);
	pxl_canvas_clear(&f.cnv);

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			pxl_t got = *pxl_buf_ptr(&f.pb, x, y);
			ST_CHECK(got == COLOR_WHITE, "pixel (%d,%d) should be white (fast path), got 0x%08X", x, y, got);
		}
	}

	fixture_deinit(&f);
}

/* Main ------------------------------------------------------------------------- */

int
main(int argc, char *argv[]) {
	ST_GETOPTS(argc, argv);
	return ST_RUN(
		/* Life-cycle & accessors */
		ST_T(test_canvas_init_basic),
		ST_T(test_canvas_set_color),
		ST_T(test_canvas_set_scissor),
		ST_T(test_canvas_set_scissor_clipped),
		ST_T(test_canvas_reset_scissor),
		
		/* offset */
		ST_T(test_canvas_offset_init_zero),
		ST_T(test_canvas_set_offset),
		ST_T(test_canvas_reset_offset),

		/* canvas_clear */
		ST_T(test_canvas_clear_white),
		ST_T(test_canvas_clear_full_black),
		ST_T(test_canvas_clear_full_white)
	);
}
